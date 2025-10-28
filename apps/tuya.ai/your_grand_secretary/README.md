
# 「Grand Secretary」 Based on T5AI-Board
## Usage
### Testing Functionality
- After network configuration, obtain the local IP address of the development board, for example: 192.168.31.110

- Run the following curl command to test whether communication is working properly:
``` curl
curl -X POST http://192.168.31.110:8080/api/message \
       -H "Content-Type: application/json" \
       -d '{"msg":"Memorial: [Tuya Smart] 185280, Tuya Smart dynamic login verification code. Staff will never ask for it, please do not disclose it to anyone."}'
```

- After sending, the development board will broadcast (read aloud) the SMS message content.

### Configuring Mobile Shortcuts

- Import the shortcut: https://www.icloud.com/shortcuts/c6374ac3282848c383747ede41050bdb

- Modify the IP address in the shortcut to the actual device address.

- In Shortcuts, create an automation:
Messages → Message Contains → input `[` → Run Immediately → Next → Select the shortcut you just imported.

- Send a message like

`[Tuya Smart] 185280, Tuya Smart dynamic login verification code. Staff will never ask for it, please do not disclose it to anyone.
to your phone to verify that the automation runs correctly.`

### Code Modification
Reverse the Display Orientation

- In `/Users/wxl/Documents/TuyaOpen/boards/T5AI/TUYA_T5AI_BOARD/tuya_t5ai_ex_module.h`, modify the line:

`#define BOARD_LCD_ROTATION   TUYA_DISPLAY_ROTATION_90`