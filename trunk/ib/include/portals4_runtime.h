#include <mpi.h>

#define PORTALS4_RUNTIME_IS_MPI

/*
 *  Private runtime, for portals4 over IB only. Used to support the
 *  public runtime.
 */

/* Values for rank under test, enables rank output prior to get_id */
extern int                      ptl_test_rank;

/* Control for logging output */
extern int                      ptl_log_level;

/* Retrieve the physical->logical mapping.
 * Note: something close should be in the public runtime eventually. */
extern ptl_process_t *get_desired_mapping(ptl_handle_ni_t ni);

/* Keep rank and mapping size. */
extern int shmemtest_rank;
extern int shmemtest_map_size;


/*
 * Public runtime, compatible with the shmem implementation.
 */

void runtime_init(void);

static inline void runtime_barrier(void)
{
	MPI_Barrier(MPI_COMM_WORLD);
}

static inline int runtime_get_rank(void)
{
	runtime_init();
	return shmemtest_rank;
}

static inline int runtime_get_size(void)
{
	runtime_init();
	return shmemtest_map_size;
}
