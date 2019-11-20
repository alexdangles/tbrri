/*
Dark current Decay for a fixed applied Voltage
.........................................................................................
Parameters:
	Vinit		--> initial applied voltage
	Vapplied		--> final applied voltage
	RampUpTime		--> time (in seconds) to reach applied voltage
	Tdelay		--> delay between measurements in ms
	nmeas		--> total number of measurements
NOTE:
	the true time of each measurement is recorded. The given delay may not be correctly
	followed due to the electrometer's triggering and data transfer requirements
****************************************************************************************/

int DarkCurrentDecay(char *fname, int Vinit, int Vapplied, int RampUpTime, int Tdelay_ms, int Vlimit, int nmeas, double *Offset)
{
int rv=0;
int flgALARM = 0;
double dc=0;
int checker = 1;
char stringr[100];
int nact = 0;

// Reset Power Supply
	rv = PS350_HVOFF(PSaddr);
	GPIB_wait(8000);
	PS350_DeviceClear(PSaddr);
	GPIB_wait(2000);
}
// setup polarity
//------------------------------------------------------------------
int pol = PS350_GetPolarity(PSaddr);
	if	((pol<0 && (Vinit>0 || Vapplied>0)) || (pol>0 && (Vinit<0 || Vapplied<0)))
		{
		INFO("WARNING *** Power Supply Polarity disagrees with voltage settings");
		GPIB_wait(1000);
		}

	// Fix polarities
	pol = PS350_GetPolarity(PSaddr);
	if(pol < 0) { if(Vinit>0) Vinit *= -1; if(Vapplied>0) Vapplied *= -1; }
	if(pol > 0) { if(Vinit<0) Vinit *= -1; if(Vapplied<0) Vapplied *= -1; }

// Make sure the power supply is able to output the required voltage
//------------------------------------------------------------------
	INFO("Resetting Power Supply");
	PS350_DeviceClear(PSaddr);
	GPIB_wait(2000);

// Set the voltage limit of the unit
	rv = PS350_SetLimitVoltage(PSaddr, Vlimit);

	if(CheckLimitVoltage(PSaddr,Vinit))	return 1;
	if(CheckLimitVoltage(PSaddr,Vapplied))	return 1;


// open output file
//-------------------------------------------------------
	FILE *fp = fopen(fname, "wt");
	if(!fp) return 1;


// setup electrometer
//-------------------------------------------------------
	double ofs=0;
	if(EL617_SetFunctionAmps(ELaddr))	{ fclose(fp); return 1; }	//Set to amps measurements
	if(InitElectrometer(ELaddr))		{ fclose(fp); return 1; }	// Initialize the Electrometer
	if(GetElectrometerOffset(ELaddr, ofs,10,10))	{ fclose(fp); return 1; }	// Measure Electrometer offset

	if(Offset) *Offset = ofs;				// return the offset to caller


// Initialize power supply
//-------------------------------------------------------
	INFO("Initializing Power Supply");
	rv = PS350_SetVoltage(PSaddr, 0);
	GPIB_wait(2000);
	rv = PS350_HVON(PSaddr);
	GPIB_wait(2000);
	GPIB_write(PSaddr, "*ESR?4\n");
	GPIB_read(PSaddr, stringr, 99, nact);

// ramp-up the supply to the required applied voltage
//-------------------------------------------------------
	PS350_SetVoltage(PSaddr, Vinit);
	rv = PS350_RampVoltage(PSaddr, 0, Vapplied, 1*pol, RampUpTime);
	if(rv) { fclose(fp); return 1; }


// Measure Dark Current
//-------------------------------------------------------

int t0 = clock();
int Tstamp;

for(int i=0, t=0 ; i<nmeas; i++, t+=Tdelay_ms) {

	while(clock() - t0 < t);

	Tstamp = clock() - t0;

	// get reading
	dc = EL617_Read(ELaddr);
	dc = fabs(dc - ofs);

	// save to file and display on screen
	fprintf(fp,		"\n%10.2f\t%10.2f",   (double)Tstamp/1000.0, dc*1e12);
	fprintf(stderr,	"\n\t%10.2f\t%10.2f", (double)Tstamp/1000.0, dc*1e12);

	fflush(fp);

	// failsafe
	if(fabs(dc) > m_Ilimit) { flgALARM = 1; break; }
}

// close file
	INFO("File saved");
	fclose(fp);

// warn if Alarm
	if(flgALARM) {
		INFO("WARNING *** Current Limit exceeded -- Shutting Down Power Supply");
		PS350_SetVoltage(PSaddr, 0);
		GPIB_wait(1000);
		}

// or inform of measurement complete
	else INFO("Measurement Complete, Ramping down Voltage -- Please Wait");

// Ramp down
	for(i=Vapplied-1; i>-1; i--) {
		PS350_SetVoltage(PSaddr, i);
		GPIB_wait(200);
		}
// turn off high voltage
	INFO("Voltage OFF");
	rv = PS350_HVOFF(PSaddr);

return 0;



