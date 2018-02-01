/* Helpers to use conf file on format used by CAEN demos */

#include "CAENConf.hpp"

/*
** Set default parameters for the acquisition
*/
void set_default_parameters(BoardParameters *params) {
    int i;
    
    params->RecordLength      = 200;	      /* Number of samples in the acquisition window (waveform mode only)    */
    params->PreTrigger        = 100;  	      /* PreTrigger is in number of samples                                  */
    params->ActiveChannel     = 0;	      /* Channel used for the data analysis (plot, histograms, etc...)       */
    params->BaselineMode      = 2;	      /* Baseline: 0=Fixed, 1=4samples, 2=16 samples, 3=64samples            */
    params->FixedBaseline     = 2100;	      /* fixed baseline (used when BaselineMode = 0)                         */
    params->PreGate           = 20;	      /* Position of the gate respect to the trigger (num of samples before) */
    params->TrgHoldOff        = 10;            /* */ 
    params->TrgMode           = 0;             /* */
    params->TrgSmoothing      = 0;             /* */
    params->SaveList          = 0;             /*  Set flag to save list events to file */
    params->DCoffset[0]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[1]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[2]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[3]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[4]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[5]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[6]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->DCoffset[7]       = 0x8000;        /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    params->ChargeSensitivity = 2;             /* Charge sesnitivity (0=max, 7=min)                    */
    params->NevAggr           = 1;             /* Number of events per aggregate (buffer). 0=automatic */
    params->AcqMode           = ACQMODE_LIST;  /* Acquisition Mode (LIST or MIXED)                     */
    params->PulsePol          = 1;             /* Pulse Polarity (1=negative, 0=positive)              */
    params->EnChargePed       = 0;             /* Enable Fixed Charge Pedestal in firmware (0=off, 1=on (1024 pedestal)) */
    params->DisTrigHist       = 0;             /* 0 = Trigger Histeresys on; 1 = Trigger Histeresys off */
    params->DefaultTriggerThr = 10;            /* Default threshold for trigger                        */

    /* NOTE: added to avoid unitialized use if not in conf file. */
    params->ChannelTriggerMask = 0xFFFFFFFF;
    params->EnableExtendedTimeStamp = 0;
    params->DisSelfTrigger = 0;
    params->TestPulsesRate = 0;
    params->EnTestPulses = 0;
    
    for(i = 0; i < 8; ++i)		
        params->GateWidth[i]         = 40;  /* Gate Width in samples                                */
    
    for(i = 0; i < 8; ++i)		
        params->DCoffset[i] = 0x8000;            /* DC offset adjust in DAC counts (0x8000 = mid scale)  */
    
    for(i = 0; i < 64; ++i) {
        params->TriggerThreshold[i] = params->DefaultTriggerThr;
    }


    
}


/*
** Read config file and assign new values to the parameters
** Returns 0 on success; -1 in case of any error detected.
*/
int load_configuration_from_file(char * fname, BoardParameters *params) {
    FILE *parameters_file;

    params->ConnectionType = CONNECTION_TYPE_AUTO;

	parameters_file = fopen(fname, "r");
	if (parameters_file != NULL) {
	while (!feof(parameters_file)) {
	   char str[100];
	   fscanf(parameters_file, "%s", str);
           if (str[0] == '#') {
              fgets(str, 100, parameters_file);
              continue;
           }

           if (strcmp(str, "AcquisitionMode") == 0) {
              char str1[100];
              fscanf(parameters_file, "%s", str1);
              if (strcmp(str1, "MIXED") == 0) 
                  params->AcqMode = ACQMODE_MIXED;
              if (strcmp(str1, "LIST")  == 0) 
                  params->AcqMode = ACQMODE_LIST;
            }

           if (strcmp(str, "ConnectionType") == 0) {
               char str1[100];
               fscanf(parameters_file, "%s", str1);
               if (strcmp(str1, "USB") == 0) 
                  params->ConnectionType = CONNECTION_TYPE_USB;
               if (strcmp(str1, "OPT")  == 0) 
                  params->ConnectionType = CONNECTION_TYPE_OPT;
           }


	   if (strcmp(str, "ConnectionLinkNum") == 0) 
             fscanf(parameters_file, "%d", &params->ConnectionLinkNum);
	   if (strcmp(str, "ConnectionConetNode") == 0) 
             fscanf(parameters_file, "%d", &params->ConnectionConetNode);
	   if (strcmp(str, "ConnectionVmeBaseAddress") == 0) 
	     fscanf(parameters_file, "%x", &params->ConnectionVMEBaseAddress);	

	   if (strcmp(str, "TriggerThreshold") == 0) {
	      int ch;
	      fscanf(parameters_file, "%d", &ch);
	      fscanf(parameters_file, "%d", &params->TriggerThreshold[ch]); 
              //printf("found TriggerThreshold %d for channel %d\n", params->TriggerThreshold[ch], ch);
	   }

	   if (strcmp(str, "RecordLength") == 0) 
              fscanf(parameters_file, "%d", &params->RecordLength);
           if (strcmp(str, "PreTrigger") == 0)
              fscanf(parameters_file, "%d", &params->PreTrigger);
           if (strcmp(str, "ActiveChannel") == 0) 
              fscanf(parameters_file, "%d", &params->ActiveChannel);
           if (strcmp(str, "BaselineMode") == 0) 
              fscanf(parameters_file, "%d", &params->BaselineMode);
           if (strcmp(str, "TrgMode") == 0) 
              fscanf(parameters_file, "%d", &params->TrgMode);
           if (strcmp(str, "TrgSmoothing") == 0) 
              fscanf(parameters_file, "%d", &params->TrgSmoothing);
           if (strcmp(str, "TrgHoldOff") == 0) 
              fscanf(parameters_file, "%d", &params->TrgHoldOff);
	   if (strcmp(str, "FixedBaseline") == 0) 
	      fscanf(parameters_file, "%d", &params->FixedBaseline);
	   if (strcmp(str, "PreGate") == 0) 
	      fscanf(parameters_file, "%d", &params->PreGate);
	   if (strcmp(str, "GateWidth") == 0) {
              int gr;
	      fscanf(parameters_file, "%d", &gr);
	      fscanf(parameters_file, "%d", &params->GateWidth[gr]);
           }

	   if (strcmp(str, "DCoffset") == 0) {
		int ch;
		fscanf(parameters_file, "%d", &ch);
		fscanf(parameters_file, "%d", &params->DCoffset[ch]);
	    }

	    if (strcmp(str, "ChargeSensitivity") == 0) 
		fscanf(parameters_file, "%d", &params->ChargeSensitivity);
	    if (strcmp(str, "NevAggr") == 0) 
		fscanf(parameters_file, "%d", &params->NevAggr);
	    if (strcmp(str, "SaveList") == 0) 
		fscanf(parameters_file, "%d", &params->SaveList);
	    if (strcmp(str, "ChannelTriggerMask") == 0) 
		fscanf(parameters_file, "%lx", &params->ChannelTriggerMask);	
	    if (strcmp(str, "PulsePolarity") == 0) 
		fscanf(parameters_file, "%d", &params->PulsePol);
	    if (strcmp(str, "EnableChargePedestal") == 0) 
		fscanf(parameters_file, "%d", &params->EnChargePed);
            if (strcmp(str, "DisableTriggerHysteresis") == 0) 
		fscanf(parameters_file, "%d", &params->DisTrigHist);
	    if (strcmp(str, "DisableSelfTrigger") == 0) 
		fscanf(parameters_file, "%d", &params->DisSelfTrigger);
	    if (strcmp(str, "EnableTestPulses") == 0) 
		fscanf(parameters_file, "%d", &params->EnTestPulses);
	    if (strcmp(str, "TestPulsesRate") == 0) 
		fscanf(parameters_file, "%d", &params->TestPulsesRate);
	    if (strcmp(str, "DefaultTriggerThr") == 0) 
		fscanf(parameters_file, "%d", &params->DefaultTriggerThr);
	    if (strcmp(str, "EnableExtendedTimeStamp") == 0) 
		fscanf(parameters_file, "%d", &params->EnableExtendedTimeStamp);
	}
		
	fclose(parameters_file);

        return 0;
	}
    return -1;
}


/* Set parameters for the acquisition */
int setup_parameters(BoardParameters *params, char *fname) {
    int ret = -1;
    set_default_parameters(params);

    ret = load_configuration_from_file(fname, params);
    return ret;
}

        
int configure_digitizer(int handle, caen::Digitizer *digitizer, BoardParameters *params) {
        int ret = 0;
        int i;
        uint32_t DppCtrl1;
        uint32_t GroupMask = 0;
        unsigned int gEquippedChannels;
        unsigned int gEquippedGroups;
        CAEN_DGTZ_BoardInfo_t gBoardInfo;

        /* stop any acquisition */
        ret = CAEN_DGTZ_SWStopAcquisition(handle);
        
        /* Reset Digitizer */
	ret |= CAEN_DGTZ_Reset(handle);       

	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration00.\n");

        gBoardInfo = caen::getRawDigitizerBoardInfo(handle);
        gEquippedChannels = gBoardInfo.Channels * 8; 
        gEquippedGroups = gEquippedChannels/8;

        for(i=0; i<gEquippedGroups; i++) {
            uint8_t mask = (params->ChannelTriggerMask>>(i*8)) & 0xFF;
            ret |= _CAEN_DGTZ_SetChannelGroupMask(handle, i, mask);
            //printf("conf: gr.%d  mask: %d\n",i,mask);
            if (mask)
                GroupMask |= (1<<i);
        }

	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration03.\n");

        ret |= CAEN_DGTZ_SetGroupEnableMask(handle, GroupMask);
        //printf("conf: GrMask: %d\n",GroupMask);

	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration04.\n");


        /* 
        ** Set selfTrigger threshold 
        ** Check if the module has 64 (VME) or 32 channels available (Desktop NIM) 
        */
        uint32_t verify;
        if ((gBoardInfo.FormFactor == 0) || (gBoardInfo.FormFactor == 1))
            for(i=0; i<64; i++) {
		ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);
                //printf("set TriggerThreshold %d for channel %d\n", params->TriggerThreshold[i], i);
            }
        
        else
            for(i=0; i<32; i++)
                ret |= _CAEN_DGTZ_SetChannelTriggerThreshold(handle,i,params->TriggerThreshold[i]);
        
        //printf("conf: set trigger threshold for %d first channels\n",i);
        
	/* Disable Group self trigger for the acquisition (mask = 0) */
	ret |= CAEN_DGTZ_SetGroupSelfTrigger(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY,0x00);    
	/* Set the behaviour when a SW tirgger arrives */
	ret |= CAEN_DGTZ_SetSWTriggerMode(handle,CAEN_DGTZ_TRGMODE_ACQ_ONLY);     
	/* Set the max number of events/aggregates to transfer in a sigle readout */
	ret |= CAEN_DGTZ_SetMaxNumAggregatesBLT(handle, MAX_AGGR_NUM_PER_BLOCK_TRANSFER);                    
	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration10.\n");

	/* Set the start/stop acquisition control */
        ret |= CAEN_DGTZ_SetAcquisitionMode(handle,CAEN_DGTZ_SW_CONTROLLED);             
        
        /* Trigger Hold Off */
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x8074, params->TrgHoldOff);
        
        /* DPP Control 1 register */
        DppCtrl1 = (((params->DisTrigHist & 0x1)      << 30)   | 
                    ((params->DisSelfTrigger & 1)     << 24)   |
                    ((params->BaselineMode & 0x7)     << 20)   |
                    ((params->TrgMode & 3)            << 18)   |
                    ((params->ChargeSensitivity & 0x7)<<  0)   | 
                    ((params->PulsePol & 1)           << 16)   | 
                    ((params->EnChargePed & 1)        <<  8)   | 
                    ((params->TestPulsesRate & 3)     <<  5)   |
                    ((params->EnTestPulses & 1)       <<  4)   |
                    ((params->TrgSmoothing & 7)       << 12));

	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8040, DppCtrl1);

        /* Set Pre Trigger (in samples) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x803C, params->PreTrigger);  

        /* Set Gate Offset (in samples) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8034, params->PreGate);     
    
        /* Set Baseline (used in fixed baseline mode only) */
	ret |= CAEN_DGTZ_WriteRegister(handle, 0x8038, params->FixedBaseline);        

	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration20.\n");

        /* Set Gate Width (in samples) */
        for(i=0; i<gEquippedGroups; i++) {
    	    ret |= CAEN_DGTZ_WriteRegister(handle, 0x1030 + 0x100*i, params->GateWidth[i]);
        }

        /* Set the waveform lenght (in samples) */
        /* NOTE: use our own exposed funtion instead of pulling in the
         * custom one */
        //ret |= _CAEN_DGTZ_SetRecordLength(handle,params->RecordLength);
        printf("Setting RecordLength to %d.\n", params->RecordLength);        
        digitizer->setRecordLength(params->RecordLength);

	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration30.\n");        

        /* Set DC offset */
	for (i=0; i<4; i++) {
            uint32_t val=42, tmp=42;
            //tmp |= CAEN_DGTZ_GetGroupDCOffset(handle, i, &val);
            printf("before set DCOffset value is %d for channel %d (%d %d)\n", val, i, tmp, ret);
            ret |= CAEN_DGTZ_SetGroupDCOffset(handle, i, params->DCoffset[i]);
        }
        /*
        digitizer->setGroupDCOffset(params->DCoffset[i]);
        val = digitizer->getGroupDCOffset();
        printf("after set DCOffset value is %d for all channels\n",
        val);
        */

        
	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration32.\n");

        /* enable Charge mode */
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00080000); 

        /* enable Timestamp */
        ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00040000);             
     
	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration40.\n");

        /* set Scope mode */
	if (params->AcqMode == ACQMODE_MIXED)
           ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 0x00010000);         
	else
           ret |= CAEN_DGTZ_WriteRegister(handle, 0x8008, 0x00010000);             
        
        /* Set number of events per memory buffer */
        /* NOTE: use our own exposed funtion instead of pulling in the
         * custom one */
        //_CAEN_DGTZ_DPP_QDC_SetNumEvAggregate(handle, params->NevAggr);        
        digitizer->setNumEventsPerAggregate(params->NevAggr);
        
	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration50.\n");

	/* enable test pulses on TRGOUT/GPO */
	if (ENABLE_TEST_PULSE) {
            uint32_t d32;
            ret |= CAEN_DGTZ_ReadRegister(handle, 0x811C, &d32);  
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x811C, d32 | (1<<15));         
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x8168, 2);         
	}
        
	if(ret != CAEN_DGTZ_Success)
            printf("Errors during Digitizer Configuration60.\n");

        /* Set Extended Time Stamp if enabled*/ 
        if (params->EnableExtendedTimeStamp)
            ret |= CAEN_DGTZ_WriteRegister(handle, 0x8004, 1 << 17); 
        
        /* Check errors */
	if(ret != CAEN_DGTZ_Success) {
            printf("Errors during Digitizer Configuration.\n");
            return -1;
        }
        return 0;
}

int dump_configuration(int handle, char *fname) 
{
    int i, ret = 0;
    
//////////////////////////////////////////////////////////////
//
// register dump, this part you can delete is for test only.
  FILE *fout;
  fout = fopen(fname, "w");

    uint32_t out;	
    for(i=0; i<8; i++){ 
       uint32_t address = 0x1030 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X) GateWidth\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }
    for(i=0; i<8; i++){ 
       uint32_t address = 0x1034 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X)  GateOffset\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }

    for(i=0; i<8; i++){ 
       uint32_t address = 0x1038 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X) GateBaseline\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }

    for(i=0; i<8; i++){ 
       uint32_t address = 0x103C + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X)  PreTrigger\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }

    for(i=0; i<8; i++){ 
       uint32_t address = 0x1074 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X) TriggerHoldOff\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }
  
/*
    for(i=0; i<8; i++){ 
       uint32_t address = 0x1078 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X)  ShapedTrigger\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }
*/
    for(i=0; i<8; i++){ 
       uint32_t address = 0x1098 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X)  DCoffset\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }

    for(i=0; i<8; i++){ 
       uint32_t address = 0x10A8 + 0x100*i;	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X)  ChannelEnableMask\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }


    for(i=0; i<64; i++){
       uint32_t address = 0x1000 + 0x100*(i/8) + 0xD0 + 4*(i%8);	    
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       fprintf(fout, "(0x%X) TriggerThreshold\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");
    }

    uint32_t address = 0x8000;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) Board Configuration\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x800C;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out);
    fprintf(fout, "(0x%X) AggregateOrganization\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x8020;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out);
    fprintf(fout, "(0x%X) EventsPerAggregate\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x811C;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out);
    fprintf(fout, "(0x%X) Front Panel I/O Control\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

  //  address = 0x8024;
  //  ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
  //  if(ret != CAEN_DGTZ_Success) {
  //     fprintf(fout, "(0x%X) Errors during register dump --------- ret(%d)\n", address, ret);
  //     ret = 0;
  //  }
  //  else fprintf(fout, "(0x%X) RecordLength\t0x%X\n", address, out);

   
    for(i = 0; i < 8; i++){ 
       address = 0x1040 + 0x100*i;
       ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
       if(ret != CAEN_DGTZ_Success) { fprintf(fout, "(0x%X) Errors during register dump --------- ret(%d)\n", address, ret); ret = 0;}
       else fprintf(fout, "(0x%X) DPPControl gr%d\t0x%X\n",address, i, out);
    }

    address = 0x8100;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) AcqControl\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x810C;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) GlobalTriggerMask\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x8120;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) GroupEnableMask\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0x814C;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) EventSize\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0xEF00;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) ReadoutControl\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    address = 0xEF1C;
    ret |= CAEN_DGTZ_ReadRegister(handle, address, &out); 
    fprintf(fout, "(0x%X) AggregateNumber per BLT\t0x%X\n", address, out);
    if(ret != CAEN_DGTZ_Success) fprintf(fout, "Errors during register dump.\n");

    fclose(fout);

    return 0;
}
