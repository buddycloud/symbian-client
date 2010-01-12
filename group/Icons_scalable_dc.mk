# ============================================================================
#  Name     : Icons_scalable_dc.mk
#  Part of  : Buddycloud
#
#  Description: This is file for creating .mif file (scalable icon)
# 
# ============================================================================


ifeq (WINS,$(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

TARGETDIR=$(ZDIR)\resource\apps

ICONTARGETFILENAME=$(TARGETDIR)\Buddycloud.mif
SETUPTARGETFILENAME=$(TARGETDIR)\Buddycloud_Setup.mif
TABTARGETFILENAME=$(TARGETDIR)\Buddycloud_Tab.mif
LISTTARGETFILENAME=$(TARGETDIR)\Buddycloud_List.mif
ARROWTARGETFILENAME=$(TARGETDIR)\Buddycloud_Arrow.mif
TOOLBARTARGETFILENAME=$(TARGETDIR)\Buddycloud_Toolbar.mif

ICONDIR=..\gfx

ICONMBG=/H..\inc\BuddycloudIconIds.h
SETUPMBG=/H..\inc\SetupIconIds.h
TABMBG=/H..\inc\TabIconIds.h
LISTMBG=/H..\inc\ListIconIds.h
ARROWMBG=/H..\inc\ArrowIconIds.h
TOOLBARMBG=/H..\inc\ToolbarIconIds.h

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing

BLD : do_nothing

CLEAN : do_nothing

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :	
	mifconv $(ICONTARGETFILENAME) $(ICONMBG) \
		/c32 $(ICONDIR)\Buddycloud.svg /c8 $(ICONDIR)\Buddycloud_Grey.svg
	mifconv $(SETUPTARGETFILENAME) $(SETUPMBG) \
		/c8 $(ICONDIR)\Logo.svg /c8 $(ICONDIR)\Splash.svg /c8 $(ICONDIR)\Tick.svg
	mifconv $(TABTARGETFILENAME) $(TABMBG) \
		/c8 $(ICONDIR)\TabContact.svg /c8 $(ICONDIR)\TabPlace.svg /c8 $(ICONDIR)\TabExplore.svg /c8 $(ICONDIR)\TabAccount.svg /c8 $(ICONDIR)\TabPreferences.svg /c8 $(ICONDIR)\TabNotifications.svg /c8 $(ICONDIR)\TabBeacons.svg
	mifconv $(LISTTARGETFILENAME) $(LISTMBG) \
		/c8 $(ICONDIR)\Phone.svg /c8 $(ICONDIR)\Letter.svg /c8 $(ICONDIR)\ListContact.svg /c8 $(ICONDIR)\ListChannel.svg /c8 $(ICONDIR)\ListPlace.svg /c8 $(ICONDIR)\Facebook.svg /c8 $(ICONDIR)\Twitter.svg
	mifconv $(ARROWTARGETFILENAME) $(ARROWMBG) \
		/c8 $(ICONDIR)\Arrow1.svg /c8 $(ICONDIR)\Arrow2.svg
	mifconv $(TOOLBARTARGETFILENAME) $(TOOLBARMBG) \
		/c8 $(ICONDIR)\ToolbarChannelPost.svg /c8 $(ICONDIR)\ToolbarPrivateMessage.svg /c8 $(ICONDIR)\ToolbarMessageReply.svg /c8 $(ICONDIR)\ToolbarSearch.svg /c8 $(ICONDIR)\ToolbarUnreadJump.svg  /c8 $(ICONDIR)\ToolbarMessagePost.svg /c8 $(ICONDIR)\ToolbarBookmarkPlace.svg
		
FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo $(ICONTARGETFILENAME)
	@echo $(SETUPTARGETFILENAME)
	@echo $(TABTARGETFILENAME)
	@echo $(LISTTARGETFILENAME)
	@echo $(ARROWTARGETFILENAME)
	@echo $(TOOLBARTARGETFILENAME)

FINAL : do_nothing

