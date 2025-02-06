# otpsh
## OTP login shell (without PAM)

use as shell replacement (calling *real* shell from config)

Config file example: **~/.otpsh**

*secret=MRSWCZBAMJ4XIZLTEBUGK4TF*

*command=/bin/bash*

Secret is base32 encoded as from: https://emn178.github.io/online-tools/base32_encode.html

**to test:**
*git clone https://github.com/NethNick/otpsh/*
*cd otpsh*
*make test*
