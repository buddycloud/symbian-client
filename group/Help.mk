# ============================================================================
#  Name     : Help.mk
#  Part of  : Buddycloud
#
#  Description: This is file for creating .hlp file (help file)
# 
# ============================================================================

MAKMAKE :
	cshlpcmp ..\help\Buddycloud.cshlp
	
	copy ..\help\releases\Buddycloud.hlp.hrh ..\inc
 
ifeq (WINS, $(findstring WINS, $(PLATFORM)))
	copy ..\help\releases\Buddycloud.hlp $(EPOCROOT)epoc32\$(PLATFORM)\c\resource\help
endif
 
CLEAN :
	del ..\help\releases\Buddycloud.hlp
	del ..\help\releases\Buddycloud.hlp.hrh
 
BLD :
	cshlpcmp ..\help\Buddycloud.cshlp
 	
	copy ..\help\releases\Buddycloud.hlp.hrh ..\inc
 
ifeq (WINS, $(findstring WINS, $(PLATFORM)))
	copy ..\help\releases\Buddycloud.hlp $(EPOCROOT)epoc32\$(PLATFORM)\c\resource\help
endif
 
FREEZE LIB CLEANLIB FINAL RESOURCE SAVESPACE RELEASEABLES : 
