/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#ifndef GMP_DESCR_H
#define GMP_DESCR_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <linux/dvb/frontend.h>


/* DVB-T/T2, DVB-S/S2, DVB-C ( A/B/C ), ATSC, DTMB */
/* Not support ISDB-T/S/C */


struct DvbTypes { const char *param; bool descr; };

const struct DvbTypes dvbt_props_n[] =
{
    { "Frequency  MHz", FALSE },
    { "Bandwidth  Hz",  FALSE },
    { "Modulation",     TRUE  },
    { "Inversion",      TRUE  },
    { "Code Rate HP",   TRUE  },
    { "Code Rate LP",   TRUE  },
    { "Transmission",   TRUE  },
    { "Guard interval", TRUE  },
    { "Hierarchy",      TRUE  },
    { "Stream ID",      FALSE }
};

const struct DvbTypes dvbs_props_n[] =
{
    { "DiSEqC ",          TRUE  },
    { "Frequency  MHz",   FALSE },
    { "Symbol rate  kBd", FALSE },
    { "Modulation",       TRUE  },
    { "Polarity",         TRUE  },
    { "Inner Fec",        TRUE  },
    { "Pilot",            TRUE  },
    { "Rolloff",          TRUE  },
    { "Stream ID",        FALSE },
    { "LNB ",             TRUE  }
};

const struct DvbTypes dvbc_props_n[] =
{
    { "Frequency  MHz",   FALSE },
    { "Symbol rate  kBd", FALSE },
    { "Modulation",       TRUE  },
    { "Inversion",        TRUE  },
    { "Inner Fec",        TRUE  },
    //{ "Stream ID",        FALSE }
};

const struct DvbTypes atsc_props_n[] =
{
    { "Frequency  MHz", FALSE },
    { "Modulation",     TRUE  }
};

const struct DvbTypes dtmb_props_n[] =
{
    { "Frequency  MHz", FALSE },
    { "Bandwidth  Hz",  FALSE },
    { "Modulation",     TRUE  },
    { "Inversion",      TRUE  },
    { "Inner Fec",      TRUE  },
    { "Transmission",   TRUE  },
    { "Guard interval", TRUE  },
    { "Interleaving",   TRUE  }
};


struct dvb_descr_all { unsigned int descr_num; const char *dvb_v5_name; const char *text_vis; };

const struct dvb_descr_all dvb_descr_delsys_type_n[] =
{
    { SYS_UNDEFINED, 		"UNDEFINED", 	"Undefined" },
    { SYS_DVBC_ANNEX_A, 	"DVBC/A", 		"DVB-C-A"   },
    { SYS_DVBC_ANNEX_B, 	"DVBC/B", 		"DVB-C-B"   },
    { SYS_DVBT, 			"DVBT",   		"DVB-T"     },
    { SYS_DSS, 				"DSS", 			"DSS" 		},
    { SYS_DVBS, 			"DVBS",   		"DVB-S"		},
    { SYS_DVBS2, 			"DVBS2",  		"DVB-S2"	},
    { SYS_DVBH, 			"DVBH", 		"DVB-H"		},
    { SYS_ISDBT, 			"ISDBT",  		"ISDB-T"	},
    { SYS_ISDBS, 			"ISDBS",  		"ISDB-S"	},
    { SYS_ISDBC, 			"ISDBC",  		"ISDB-C"	},
    { SYS_ATSC, 			"ATSC",   		"ATSC"		},
    { SYS_ATSCMH, 			"ATSCMH", 		"ATSC-MH"	},
    { SYS_DTMB, 			"DTMB", 		"DTMB"		},
    { SYS_CMMB, 			"CMMB", 		"CMMB"		},
    { SYS_DAB, 				"DAB", 			"DAB"		},
    { SYS_DVBT2, 			"DVBT2", 		"DVB-T2"	},
    { SYS_TURBO, 			"TURBO", 		"TURBO"		},
    { SYS_DVBC_ANNEX_C, 	"DVBC/C", 		"DVB-C-C"	}
};

const struct dvb_descr_all dvb_descr_inversion_type_n[] =
{
    { INVERSION_OFF,  "OFF",  "Off"  },
    { INVERSION_ON,   "ON",   "On"   },
    { INVERSION_AUTO, "AUTO", "Auto" }
};

const struct dvb_descr_all dvb_descr_coderate_type_n[] =
{
    { FEC_NONE, "NONE", "None" },
    { FEC_1_2,  "1/2",  "1/2"  },
    { FEC_2_3,  "2/3",  "2/3"  },
    { FEC_3_4,  "3/4",  "3/4"  },
    { FEC_4_5,  "4/5",  "4/5"  },
    { FEC_5_6,  "5/6",  "5/6"  },
    { FEC_6_7,  "6/7",  "6/7"  },
    { FEC_7_8,  "7/8",  "7/8"  },
    { FEC_8_9,  "8/9",  "8/9"  },
    { FEC_AUTO, "AUTO", "Auto" },
    { FEC_3_5,  "3/5",  "3/5"  },
    { FEC_9_10, "9/10", "9/10" },
    { FEC_2_5,  "2/5",  "2/5"  }
};

const struct dvb_descr_all dvb_descr_modulation_type_n[] =
{
    { QPSK,     "QPSK",     "QPSK"     },
    { QAM_16,   "QAM/16",   "QAM 16"   },
    { QAM_32,   "QAM/32",   "QAM 32"   },
    { QAM_64,   "QAM/64",   "QAM 64"   },
    { QAM_128,  "QAM/128",  "QAM 128"  },
    { QAM_256,  "QAM/256",  "QAM 256"  },
    { QAM_AUTO, "QAM/AUTO", "Auto"     },
    { VSB_8,    "VSB/8",    "VSB 8"    },
    { VSB_16,   "VSB/16",   "VSB 16"   },
    { PSK_8,    "PSK/8",    "PSK 8"    },
    { APSK_16,  "APSK/16",  "APSK 16"  },
    { APSK_32,  "APSK/32",  "APSK 32"  },
    { DQPSK,    "DQPSK",    "DQPSK"    },
    { QAM_4_NR, "QAM/4_NR", "QAM 4 NR" }
};

const struct dvb_descr_all dvb_descr_transmode_type_n[] =
{
    { TRANSMISSION_MODE_2K,   "2K",   "2K"   },
    { TRANSMISSION_MODE_8K,   "8K",   "8K"   },
    { TRANSMISSION_MODE_AUTO, "AUTO", "Auto" },
    { TRANSMISSION_MODE_4K,   "4K",   "4K"   },
    { TRANSMISSION_MODE_1K,   "1K",   "1K"   },
    { TRANSMISSION_MODE_16K,  "16K",  "16K"  },
    { TRANSMISSION_MODE_32K,  "32K",  "32K"  },
    { TRANSMISSION_MODE_C1,   "C1",   "C1"   },
    { TRANSMISSION_MODE_C3780,"C3780","C3780"}
};

const struct dvb_descr_all dvb_descr_guard_type_n[] =
{
    { GUARD_INTERVAL_1_32,   "1/32",   "32"     },
    { GUARD_INTERVAL_1_16,   "1/16",   "16"     },
    { GUARD_INTERVAL_1_8,    "1/8",    "8"      },
    { GUARD_INTERVAL_1_4,    "1/4",    "4"      },
    { GUARD_INTERVAL_AUTO,   "AUTO",   "Auto"   },
    { GUARD_INTERVAL_1_128,  "1/128",  "128"    },
    { GUARD_INTERVAL_19_128, "19/128", "19/128" },
    { GUARD_INTERVAL_19_256, "19/256", "19/256" },
    { GUARD_INTERVAL_PN420,  "PN420",  "PN 420" },
    { GUARD_INTERVAL_PN595,  "PN595",  "PN 595" },
    { GUARD_INTERVAL_PN945,  "PN945",  "PN 945" }
};

const struct dvb_descr_all dvb_descr_hierarchy_type_n[] =
{
    { HIERARCHY_NONE, "NONE", "None" },
    { HIERARCHY_1,    "1",    "1"    },
    { HIERARCHY_2,    "2",    "2"    },
    { HIERARCHY_4,    "4",    "4"    },
    { HIERARCHY_AUTO, "AUTO", "Auto" }

};

const struct dvb_descr_all dvb_descr_pilot_type_n[] =
{
    { PILOT_ON,   "ON",   "On"   },
    { PILOT_OFF,  "OFF",  "Off"  },
    { PILOT_AUTO, "AUTO", "Auto" }
};

const struct dvb_descr_all dvb_descr_roll_type_n[] =
{
    { ROLLOFF_35,   "35",   "35"   },
    { ROLLOFF_20,   "20",   "20"   },
    { ROLLOFF_25,   "25",   "25"   },
    { ROLLOFF_AUTO, "AUTO", "Auto" }
};

const struct dvb_descr_all dvb_descr_polarity_type_n[] =
{
    { SEC_VOLTAGE_18,  "HORIZONTAL", "H  18V" },
    { SEC_VOLTAGE_13,  "VERTICAL",   "V  13V" },
    { SEC_VOLTAGE_18,  "LEFT",       "L  18V" },
    { SEC_VOLTAGE_13,  "RIGHT",      "R  13V" },
    //{ SEC_VOLTAGE_OFF, "OFF",        "Off"    }
};

const struct dvb_descr_all dvb_descr_ileaving_type_n[] =
{
    { INTERLEAVING_NONE, "NONE", "None" },
    { INTERLEAVING_AUTO, "AUTO", "Auto" },
    { INTERLEAVING_240,  "240",  "240"  },
    { INTERLEAVING_720,  "720",  "720"  }
};

enum lnbs 
{ 
    LNB_UNV,
    LNB_DBS,
    LNB_EXT,
    LNB_STD,
    LNB_EHD,
    LNB_CBD,
    LNB_CMT,
    LNB_DSP,
    LNB_BS1
};

const struct dvb_descr_all dvb_descr_lnb_type_n[] =
{
    { LNB_UNV, "UNIVERSAL", "Universal" },
    { LNB_DBS, "DBS",	    "DBS"       },
    { LNB_EXT, "EXTENDED",  "Extended"  },
    { LNB_STD, "STANDARD",  "Standard"  },
    { LNB_EHD, "ENHANCED",  "Enhanced"  },
    { LNB_CBD, "C-BAND",    "C-Band"    },
    { LNB_CMT, "C-MULT",    "C-Mult"    },
    { LNB_DSP, "DISHPRO",   "Dishpro"   },
    { LNB_BS1, "110BS",	    "110 BS"    }
};

enum lnb_num
{
    LNB_0,
    LNB_1,
    LNB_2,
    LNB_3,
    LNB_4,
    LNB_5,
    LNB_6,
    LNB_7
};

const struct dvb_descr_all dvb_descr_lnb_num_n[] =
{
    { 99999, "NONE",    "None"  },
    { LNB_0, "LNB 1",   "Lnb 1" },
    { LNB_1, "LNB 2",   "Lnb 2" },
    { LNB_2, "LNB 3",   "Lnb 3" },
    { LNB_3, "LNB 4",   "Lnb 4" },
    { LNB_4, "LNB 5",   "Lnb 5" },
    { LNB_5, "LNB 6",   "Lnb 6" },
    { LNB_6, "LNB 7",   "Lnb 7" },
    { LNB_7, "LNB 8",	"Lnb 8" }
};

#endif // GMP_DESCR_H
