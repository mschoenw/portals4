#include "config.h"

#include "portals4.h"

#include "ptl_internal_iface.h"        
#include "ptl_internal_startup.h"        
#include "ptl_internal_global.h"
#include "ptl_internal_error.h"
#include "ptl_internal_nit.h"
#include "ptl_internal_EQ.h"
#include "shared/ptl_internal_handles.h"
#include "shared/ptl_command_queue_entry.h"

int PtlPTAlloc(ptl_handle_ni_t ni_handle,
               unsigned int    options,
               ptl_handle_eq_t eq_handle,
               ptl_pt_index_t  pt_index_req,
               ptl_pt_index_t *pt_index)
{
    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_cqe_t *entry;

#ifndef NO_ARG_VALIDATION
    if (PtlInternalLibraryInitialized() == PTL_FAIL) {
        VERBOSE_ERROR("Library not initialized\n");
        return PTL_NO_INIT;
    }
    if ((ni.s.ni >= 4) || (ni.s.code != 0) || 
                                    (ptl_iface.ni[ni.s.ni].refcount == 0)) {
        VERBOSE_ERROR("Invalid NI passed to PtlPTAlloc\n");
        return PTL_ARG_INVALID;
    }
    if (options & ~PTL_PT_ALLOC_OPTIONS_MASK) {
        VERBOSE_ERROR("Invalid options to PtlPTAlloc (0x%x)\n", options);
        return PTL_ARG_INVALID;
    }
    if ((eq_handle == PTL_EQ_NONE) && options & PTL_PT_FLOWCTRL) {
        return PTL_PT_EQ_NEEDED;
    }
    if (PtlInternalEQHandleValidator(eq_handle, 1)) {
        VERBOSE_ERROR("Invalid EQ passed to PtlPTAlloc\n");
        return PTL_ARG_INVALID;
    }
    if ((pt_index_req > nit_limits[ni.s.ni].max_pt_index) && 
                                            (pt_index_req != PTL_PT_ANY)) {
        VERBOSE_ERROR("Invalid pt_index request passed to PtlPTAlloc\n");
        return PTL_ARG_INVALID;
    }

    if ( (pt_index_req != PTL_PT_ANY) && 
                        pt_is_inuse( ni.s.ni, pt_index_req ) ) { 
        VERBOSE_ERROR("currently used pt_index request passed to PtlPTAlloc\n");
        return PTL_ARG_INVALID;
    }
    
    if (pt_index == NULL) {
        VERBOSE_ERROR("Invalid pt_index pointer (NULL) passed to PtlPTAlloc\n");
        return PTL_ARG_INVALID;
    }
#endif /* ifndef NO_ARG_VALIDATION */

    if ( pt_index_req == PTL_PT_ANY ) {
        *pt_index = find_pt_index(ni.s.ni); 
        if ( *pt_index == -1 ) return PTL_PT_FULL;
    } else {
        *pt_index = pt_index_req;
        mark_pt_inuse( ni.s.ni, pt_index_req ); 
    }

    ptl_cq_entry_alloc( ptl_iface_get_cq(&ptl_iface), &entry );

    entry->base.type         = PTLPTALLOC;
    entry->base.remote_id    = ptl_iface_get_rank(&ptl_iface);
    entry->ptAlloc.ni_handle = ni;
    entry->ptAlloc.options   = options;
    entry->ptAlloc.eq_handle = ( ptl_internal_handle_converter_t ) eq_handle;
    entry->ptAlloc.pt_index  =  *pt_index;

    ptl_cq_entry_send_block(ptl_iface_get_cq(&ptl_iface), 
                      ptl_iface_get_peer(&ptl_iface), 
                      entry, sizeof(ptl_cqe_ptalloc_t));

    return PTL_OK;
}

int PtlPTFree(ptl_handle_ni_t ni_handle,
              ptl_pt_index_t  pt_index)
{
    int ret, cmd_ret = PTL_STATUS_LAST;
    const ptl_internal_handle_converter_t ni = { ni_handle };
    ptl_cqe_t *entry;

#ifndef NO_ARG_VALIDATION
    if (PtlInternalLibraryInitialized() == PTL_FAIL) {
        VERBOSE_ERROR("Not initialized\n");
        return PTL_NO_INIT;
    }
    if ((ni.s.ni >= 4) || (ni.s.code != 0) || 
                                    (ptl_iface.ni[ni.s.ni].refcount == 0)) {
        VERBOSE_ERROR ("ni.s.ni too big (%u >= 4) or ni.s.code wrong (%u != 0)"
                        " or nit not initialized\n", ni.s.ni, ni.s.code);
        return PTL_ARG_INVALID;
    }
    if (pt_index == PTL_PT_ANY) {
        VERBOSE_ERROR("pt_index is PTL_PT_ANY\n");
        return PTL_ARG_INVALID;
    }
    if (pt_index > nit_limits[ni.s.ni].max_pt_index) {
        VERBOSE_ERROR("pt_index is too big (%u > %u)\n", pt_index,
                      nit_limits[ni.s.ni].max_pt_index);
        return PTL_ARG_INVALID;
    }

    if ( ! pt_is_inuse( ni.s.ni, pt_index ) ) { 
        VERBOSE_ERROR("pt_index is not in use\n");
        return PTL_ARG_INVALID;
    }
#endif /* ifndef NO_ARG_VALIDATION */


    ret = ptl_cq_entry_alloc( ptl_iface_get_cq(&ptl_iface), &entry );
    if (0 != ret ) return PTL_FAIL;

    entry->base.type            = PTLPTFREE;
    entry->base.remote_id       = ptl_iface_get_rank(&ptl_iface);
    entry->ptFree.ni_handle     = ni;
    entry->ptFree.pt_index      = pt_index;
    entry->ptFree.retval_ptr    = &cmd_ret;

    ret = ptl_cq_entry_send_block(ptl_iface_get_cq(&ptl_iface), 
                      ptl_iface_get_peer(&ptl_iface), 
                      entry, sizeof(ptl_cqe_ptfree_t));
    if (ret < 0 ) return PTL_FAIL;

    do {
        ret = ptl_ppe_progress(&ptl_iface, 1);
        if (ret < 0) return PTL_FAIL;
        __sync_synchronize();
    } while (PTL_STATUS_LAST == cmd_ret);

    return cmd_ret;
}


int PtlPTDisable(ptl_handle_ni_t ni_handle,
                 ptl_pt_index_t  pt_index)
{
    fprintf(stderr, "PtlPTDisable() unimplemented\n");
    return PTL_FAIL;

}


int PtlPTEnable(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index)

{
    fprintf(stderr, "PtlPTEnable() unimplemented\n");
    return PTL_FAIL;
}
