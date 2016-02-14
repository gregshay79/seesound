#include "stdafx.h"

double hilbertKernel64[64] = {

	0.00000000000000000000,
	-0.00001847387500000000,
	0.00000000000000000000,
	-0.00018272097500000002,
	0.00000000000000000000,
	-0.00057353614999999998,
	0.00000000000000000000,
	-0.00129707349999999990,
	0.00000000000000000000,
	-0.00250693224999999980,
	0.00000000000000000000,
	-0.00440736024999999980,
	0.00000000000000000000,
	-0.00725740024999999970,
	0.00000000000000000000,
	-0.01138145500000000000,
	0.00000000000000000000,
	-0.01719613000000000000,
	0.00000000000000000000,
	-0.02527583000000000300,
	0.00000000000000000000,
	-0.03650850500000000400,
	0.00000000000000000000,
	-0.05247590999999999400,
	0.00000000000000000000,
	-0.07651495250000001100,
	0.00000000000000000000,
	-0.11733732500000001000,
	0.00000000000000000000,
	-0.20712777499999999000,
	0.00000000000000000000,
	-0.63680097499999999000,
	0.00000000000000000000,
	0.63162485000000002000,
	0.00000000000000000000,
	0.20209892500000001000,
	0.00000000000000000000,
	0.11259120000000000000,
	0.00000000000000000000,
	0.07216851250000000400,
	0.00000000000000000000,
	0.04861558500000000300,
	0.00000000000000000000,
	0.03318847499999999500,
	0.00000000000000000000,
	0.02251576750000000200,
	0.00000000000000000000,
	0.01498281250000000000,
	0.00000000000000000000,
	0.00967424750000000010,
	0.00000000000000000000,
	0.00599487724999999990,
	0.00000000000000000000,
	0.00351579825000000040,
	0.00000000000000000000,
	0.00190922150000000020,
	0.00000000000000000000,
	0.00092067837499999999,
	0.00000000000000000000,
	0.00035699137499999999,
	0.00000000000000000000,
	0.00007983175750000000,
	0.00000000000000000000,
	-0.00000000000000000028,
};


double LPFKernel64[64] = {
	0.00000000000000000022,
	-0.00000069033447196868,
	0.00005680604932569600,
	0.00001928657157654552,
	-0.00023650658247075651,
	-0.00009517686538599564,
	0.00056476571720462290,
	0.00028468986477175472,
	-0.00107746866931986190,
	-0.00066914713944701278,
	0.00181160974119567260,
	0.00136118667459609850,
	-0.00279483190503131960,
	-0.00250932000357849880,
	0.00403540458466520450,
	0.00430288471375915840,
	-0.00551448722971354010,
	-0.00698308227775021160,
	0.00718210796516120280,
	0.01087107712338168000,
	-0.00895777348954445750,
	-0.01644020327507381500,
	0.01073591838262705700,
	0.02450159879854040300,
	-0.01239561044169927700,
	-0.03672979218908129600,
	0.01381326573282215200,
	0.05746991023893750800,
	-0.01487674254231089900,
	-0.10278574297519154000,
	0.01549817575588449200,
	0.31805659882934589000,
	0.48437848499005759000,
	0.31547116088083194000,
	0.01524692422191351700,
	-0.10029033130191042000,
	-0.01439545332420291400,
	0.05514580040306563800,
	0.01314215233466865100,
	-0.03464341005791124500,
	-0.01158839693820477300,
	0.02269921708202250400,
	0.00985387302384325810,
	-0.01494515720802542600,
	-0.00806254902655131020,
	0.00968397660398552340,
	0.00632908580468149060,
	-0.00608430449490987480,
	-0.00474751374762283090,
	0.00365747984987319170,
	0.00338342342733070990,
	-0.00207279200862016960,
	-0.00227100891800624100,
	0.00108583488134813030,
	0.00141479142139528830,
	-0.00050960307830825167,
	-0.00079565593293183827,
	0.00020207650419023804,
	0.00037996656783853143,
	-0.00005924166445975763,
	-0.00013001955304578464,
	0.00000842637980985989,
	0.00001405205413001567,
	0.00000000000000000001,
	
};

double BP500Kernel64[64] = {
	-0.00000000000000000103,
	0.00006231539999999999,
	0.00019280474999999999,
	0.00023882849999999999,
	0.00000000004962187500,
	-0.00069794775000000002,
	-0.00191686499999999980,
	-0.00352842000000000000,
	-0.00517181999999999970,
	-0.00627401999999999910,
	-0.00614877750000000000,
	-0.00417154499999999960,
	-0.00000000058639125000,
	0.00621489750000000020,
	0.01369650000000000000,
	0.02105324999999999900,
	0.02646937500000000000,
	0.02806687499999999800,
	0.02437672499999999800,
	0.01480980000000000000,
	0.00000000187945500000,
	-0.01810027499999999900,
	-0.03643860000000000200,
	-0.05138984999999999400,
	-0.05949832499999999800,
	-0.05827987499999999500,
	-0.04688339999999999900,
	-0.02644252500000000100,
	-0.00000000312132000000,
	0.02800642499999999800,
	0.05260184999999999900,
	0.06929107499999999400,
	0.07499999999999999700,
	0.06872782500000000600,
	0.05174909999999999900,
	0.02732647499999999900,
	0.00000000302035500000,
	-0.02537317500000000100,
	-0.04460564999999999700,
	-0.05496929999999999900,
	-0.05562382499999999500,
	-0.04760954999999999400,
	-0.03344497500000000200,
	-0.01645425000000000000,
	-0.00000000169162500000,
	0.01319257500000000000,
	0.02148150000000000100,
	0.02445450000000000100,
	0.02278784999999999800,
	0.01789537500000000200,
	0.01148362499999999900,
	0.00513372750000000000,
	0.00000000047648700000,
	-0.00332769749999999990,
	-0.00480190499999999950,
	-0.00477813749999999970,
	-0.00381912750000000000,
	-0.00250450499999999980,
	-0.00128964000000000010,
	-0.00043442924999999999,
	-0.00000000002727975000,
	0.00010434525000000000,
	0.00004769415000000000,
	-0.00000000000000000096,



};