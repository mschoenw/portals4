#ifndef PTL_INTERNAL_DM_H
#define PTL_INTERNAL_DM_H

#include "ptl_visibility.h"

void INTERNAL PtlInternalDMSetup(void);
void INTERNAL PtlInternalDMTeardown(void);
int INTERNAL  PtlInternalAmITheCatcher(void);
/* note: this is not internal so that Yod can use it */
void PtlInternalDMStop(void);

#endif // ifndef PTL_INTERNAL_DM_H
/* vim:set expandtab: */