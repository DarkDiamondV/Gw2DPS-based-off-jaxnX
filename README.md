# Changes by me
Changed the stay on top to work more consistently.

# GW2DPS
Based on JaxnX, modified by Kozzy.5974, DoM.8396 & Tyrox.1352

What makes it legal : It does not offend the Terms of Service of Guild Wars 2. It does not read from game memory. Instead it uses Computer Vision techniques to read out numbers presented in the combat log by taking screenshots of the chat block periodically, converting pixels of the image into numbers and using the converted numbers for dmg / dps calculation. The whole process happens in real time.

Settings for correct measuring : 
 - Options/Graphics Options: 
    - Interface Size=Normal
    - Resolution=Windowed Fullscreen
 - Chatbox/options: 
    - Text Size=Medium
    - Disable Timestamps
 - Chatbox/Combat Page/Options: Only Enable:
    - Outgoing Buff Damage
    - Outgoing Damage
    - Outgoing Mitigated Damage. 
 - Make sure your combat log has more then 16+ lines and always visible.

Compiled in Qt Creator written in C++

Your name can only be 10 characters long.
Get IP from Server Hoster.

Don't mess up configurations within the tool.

To make it work:
 - Windowed Fullscreen
 - Interface-size: normal
 - text-size: medium
 - timestamps: off
 - Combat-Log active
 
Download Link: Server & Client
Windows: http://gw2dps.com/download
Mac: Can't compile it at the moment. If it works without any big issues after we tried it, find the link here.
