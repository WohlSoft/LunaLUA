#include "SafeFPUControl.h"

#include <float.h>
#include <iostream>
#include <cfenv>

#pragma fenv_access (on)
#pragma STDC FENV_ACCESS ON


/*
 FIXME: Right now disabling all FPU exceptions will crash SMBX with the SSE2 error 0xc00002b4 by _lj_set_tab function by luajit.
*/

SafeFPUControl::SafeFPUControl()
{
    /*
    int err = _controlfp_s(&fpu_control, 0, _MCW_EM);
    if (err) {
        std::cout << "ERROR: Failed to get and set FPU control value: code = " << err << std::endl;
    } 
    */
}


SafeFPUControl::~SafeFPUControl()
{
    std::feclearexcept(FE_ALL_EXCEPT);
    /*
    unsigned int temp;
    int err = _controlfp_s(&temp, fpu_control, _MCW_EM);
    if (err) {
        std::cout << "ERROR: Failed to restore FPU control value: code = " << err << std::endl;
    }
    */
}