/* The API definition */
#include <portals4.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* System libraries */
#include <stddef.h>		       /* for NULL */
#include <sys/mman.h>		       /* for mmap() and shm_open() */
#include <sys/stat.h>		       /* for S_IRUSR and friends */
#include <fcntl.h>		       /* for O_RDWR */
#include <stdlib.h>		       /* for getenv() */
#include <unistd.h>		       /* for close() */
#include <limits.h>		       /* for UINT_MAX */
#include <string.h>		       /* for memset() */

/* Internals */
#include "portals4_runtime.h"
#include "ptl_visibility.h"
#include "ptl_internal_assert.h"
#include "ptl_internal_atomic.h"
#include "ptl_internal_commpad.h"
#include "ptl_internal_nit.h"
#include "ptl_internal_fragments.h"
#include "ptl_internal_nemesis.h"
#include "ptl_internal_papi.h"

volatile char *comm_pad = NULL;
size_t num_siblings = 0;
size_t proc_number = 0;
size_t per_proc_comm_buf_size = 0;
size_t firstpagesize = 0;

static unsigned int init_ref_count = 0;
static size_t comm_pad_size = 0;
static const char *comm_pad_shm_name = NULL;

#define PARSE_ENV_NUM(env_str, var, reqd) do { \
    char * strerr; \
    const char *str = getenv(env_str); \
    if (str == NULL) { \
	if (reqd == 1) goto exit_fail; \
    } else { \
	size_t tmp = strtol(str, &strerr, 10); \
	if (strerr == NULL || strerr == str || *strerr != 0) { \
	    goto exit_fail; \
	} \
	var = tmp; \
    } \
} while (0)

/* The trick to this function is making it thread-safe: multiple threads can
 * all call PtlInit concurrently, and all will wait until initialization is
 * complete, and if there is a failure, all will report failure.
 *
 * PtlInit() will only work if the process has been executed by yod (which
 * handles important aspects of the init/cleanup and passes data via
 * envariables).
 */
int API_FUNC PtlInit(
    void)
{
    unsigned int race = PtlInternalAtomicInc(&init_ref_count, 1);
    static volatile int done_initializing = 0;
    static volatile int failure = 0;

    if (race == 0) {
	int shm_fd;

#ifdef _SC_PAGESIZE
	firstpagesize = sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
	firstpagesize = sysconf(_SC_PAGE_SIZE);
#elif defined(HAVE_GETPAGESIZE)
	firstpagesize = getpagesize();
#else
	firstpagesize = 4096;
#endif

	/* Parse the official yod-provided environment variables */
	comm_pad_shm_name = getenv("PORTALS4_SHM_NAME");
	if (comm_pad_shm_name == NULL) {
	    goto exit_fail;
	}
	PARSE_ENV_NUM("PORTALS4_NUM_PROCS", num_siblings, 1);
	PARSE_ENV_NUM("PORTALS4_RANK", proc_number, 1);
	PARSE_ENV_NUM("PORTALS4_COMM_SIZE", per_proc_comm_buf_size, 1);
	PARSE_ENV_NUM("PORTALS4_SMALL_FRAG_SIZE", SMALL_FRAG_SIZE, 0);
	PARSE_ENV_NUM("PORTALS4_LARGE_FRAG_SIZE", LARGE_FRAG_SIZE, 0);
	PARSE_ENV_NUM("PORTALS4_SMALL_FRAG_COUNT", SMALL_FRAG_COUNT, 0);
	PARSE_ENV_NUM("PORTALS4_LARGE_FRAG_COUNT", LARGE_FRAG_COUNT, 0);
	assert(((SMALL_FRAG_COUNT * SMALL_FRAG_SIZE) +
		(LARGE_FRAG_COUNT * LARGE_FRAG_SIZE) +
		sizeof(NEMESIS_blocking_queue)) ==
	       per_proc_comm_buf_size);

	comm_pad_size = firstpagesize + (per_proc_comm_buf_size * (num_siblings + 1));	// the one extra is for the collator

	memset(&nit, 0, sizeof(ptl_internal_nit_t));
	nit_limits.max_mes = 128 * 4;  // Thus, the ME/LE list for each NI can be maxed out
	nit_limits.max_over = 128;     // Arbitrary
	nit_limits.max_mds = 128;      // Arbitrary
	nit_limits.max_cts = 128;      // Arbitrary
	nit_limits.max_eqs = 128;      // Arbitrary
	nit_limits.max_pt_index = 63;  // Minimum required by spec
	nit_limits.max_iovecs = 0;     // XXX: ???
	nit_limits.max_me_list = 128;  // Arbitrary
	nit_limits.max_msg_size = 0xffffffffffffffffULL;	// may need to be smaller
	nit_limits.max_atomic_size = LARGE_FRAG_SIZE - sizeof(void *) - sizeof(uint64_t);	// single payload

	/* Open the communication pad */
	assert(comm_pad == NULL);
	shm_fd = shm_open(comm_pad_shm_name, O_RDWR, S_IRUSR | S_IWUSR);
	assert(shm_fd >= 0);
	if (shm_fd < 0) {
	    //perror("PtlInit: shm_open failed");
	    goto exit_fail;
	}
	comm_pad =
	    (char *)mmap(NULL, comm_pad_size, PROT_READ | PROT_WRITE,
			 MAP_SHARED, shm_fd, 0);
	if (comm_pad == MAP_FAILED) {
	    //perror("PtlInit: mmap failed");
	    goto exit_fail;
	}
	ptl_assert(close(shm_fd), 0);
	/* Locate and initialize my fragments memory (beginning with a pointer to headers) */
	PtlInternalFragmentSetup((comm_pad + firstpagesize +
				  (per_proc_comm_buf_size * proc_number)));
#ifdef HAVE_LIBPAPI
	PtlInternalPAPIInit();
#endif

	/**************************************************************************
	 * Can Now Announce My Presence
	 **************************************************************************/
	comm_pad[proc_number] = 1;

	if (proc_number != num_siblings) {
	    /* Now, wait for my siblings to get here, unless I'm the COLLATOR. */
	    size_t i;
	    for (i = 0; i < num_siblings; ++i) {
		/* oddly enough, this should reduce cache traffic for large numbers
		 * of siblings */
		while (comm_pad[i] == 0) ;
	    }
	}

	/* Release any concurrent initialization calls */
	__sync_synchronize();
	done_initializing = 1;
        runtime_init();
        return PTL_OK;
    } else {
	/* Should block until other inits finish. */
	while (!done_initializing) ;
	if (!failure)
            return PTL_OK;
	else
	    goto exit_fail_fast;
    }
  exit_fail:
    failure = 1;
    __sync_synchronize();
    done_initializing = 1;
  exit_fail_fast:
    PtlInternalAtomicInc(&init_ref_count, -1);
    return PTL_FAIL;
}

void API_FUNC PtlFini(
    void)
{
    unsigned int lastone;

    runtime_finalize();
    assert(init_ref_count > 0);
    if (init_ref_count == 0)
	return;
    lastone = PtlInternalAtomicInc(&init_ref_count, -1);
    if (lastone == 1) {
	/* Clean up */
#ifdef HAVE_LIBPAPI
	PtlInternalPAPITeardown();
#endif
	//printf("%u MUNMAP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", (unsigned int)proc_number);
	ptl_assert(munmap((void *)comm_pad, comm_pad_size), 0);
	comm_pad = NULL;
    }
}
