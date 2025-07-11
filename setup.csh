#!/bin/tcsh

    # for root 
    #    source /apps/root/PRO/setroot_CUE.csh

    # for hcana                                                                                                                                                                                    
               
    #    setenv HCANA /home/cdaq/nps-2023/hcana
    module use /capps/modulefiles
    module load hcana
    module load panguin    

    echo " Setting hcana from: "
    echo $HCANA
    echo " Edit this file if you want to use different hcana"
    #    setenv LD_LIBRARY_PATH "$LD_LIBRARY_PATH":$HCANA/lib64 
    #    setenv PATH "$PATH":$HCANA/bin 

    # for hallc_replay
    setenv DB_DIR DBASE
