REM @ECHO OFF

REM This is a sample for the custom toggle functionality.
REM Reuse at your own risk.

REM Ignore double-click curiosity
IF NOT "%1"=="magic_param" GOTO END

REM Check if we are putting or getting
IF "%2"=="" GOTO GET

:PUT

REM Request to set the lresdata.pak as active
IF "%2"=="0" GOTO PUTLRES

REM Request to set the hresdata.pak as active
IF "%2"=="1" GOTO PUTHRES

REM Invalid input.
GOTO END

:PUTLRES

REM If its not there, then it probably is already active
IF NOT EXIST lresdata.pak GOTO GET

REM If the hresdata is already there, but lresdata isn't, then something broke
IF EXIST hresdata.pak GOTO END

REM If the hresdata isn't there, then something broke
IF NOT EXIST data.pak GOTO END

REM Disable hresdata
MOVE data.pak hresdata.pak

REM Enable lresdata
MOVE lresdata.pak data.pak

REM Report back the results
GOTO GET

:PUTHRES

REM If its not there, then it probably is already active
IF NOT EXIST hresdata.pak GOTO GET

REM If the lresdata is already there, but hresdata isn't, then something broke
IF EXIST lresdata.pak GOTO END

REM If the lresdata isn't there, then something broke
IF NOT EXIST data.pak GOTO END

REM Disable lresdata
MOVE data.pak lresdata.pak

REM Enable hresdata
MOVE hresdata.pak data.pak

REM Report back the results
GOTO GET

:GET

REM If the hresdata.pak is there, then the lresdata must be active
IF EXIST hresdata.pak EXIT 0

REM If the lresdata.pak is there, then the hresdata must be active
IF EXIST lresdata.pak EXIT 1

:END

REM Unexpected error
EXIT 2
