/*
 * auth.c - deal with authentication.
 *
 * This file implements the VNC authentication protocol when setting up an RFB
 * connection.
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

/*
 * rfbAuthNewClient is called when we reach the point of authenticating
 * a new client.  If authentication isn't being used then we simply send
 * rfbNoAuth.  Otherwise we send rfbVncAuth plus the challenge.
 */

void
rfbAuthNewClient(cl)
    rfbClientPtr cl;
{
    char buf[4 + CHALLENGESIZE];
    int len;

    cl->state = RFB_AUTHENTICATION;

    if (cl->screen->authPasswdData && !cl->reverseConnection) {
        *(uint32_t *)buf = Swap32IfLE(rfbVncAuth);
        rfbRandomBytes(cl->authChallenge);
        memcpy(&buf[4], (char *)cl->authChallenge, CHALLENGESIZE);
        len = 4 + CHALLENGESIZE;
    } else {
        *(uint32_t *)buf = Swap32IfLE(rfbNoAuth);
        len = 4;
        cl->state = RFB_INITIALISATION;
    }

    if (rfbWriteExact(cl, buf, len) < 0) {
        rfbLogPerror("rfbAuthNewClient: write");
        rfbCloseClient(cl);
        return;
    }
}


/*
 * rfbAuthProcessClientMessage is called when the client sends its
 * authentication response.
 */

void
rfbAuthProcessClientMessage(cl)
    rfbClientPtr cl;
{
    int n;
    uint8_t response[CHALLENGESIZE];
    uint32_t authResult;

    if ((n = rfbReadExact(cl, (char *)response, CHALLENGESIZE)) <= 0) {
        if (n != 0)
            rfbLogPerror("rfbAuthProcessClientMessage: read");
        rfbCloseClient(cl);
        return;
    }

    if(!cl->screen->passwordCheck(cl,(const char*)response,CHALLENGESIZE)) {
        rfbErr("rfbAuthProcessClientMessage: password check failed\n");
        authResult = Swap32IfLE(rfbVncAuthFailed);
        if (rfbWriteExact(cl, (char *)&authResult, 4) < 0) {
            rfbLogPerror("rfbAuthProcessClientMessage: write");
        }
        rfbCloseClient(cl);
        return;
    }

    authResult = Swap32IfLE(rfbVncAuthOK);

    if (rfbWriteExact(cl, (char *)&authResult, 4) < 0) {
        rfbLogPerror("rfbAuthProcessClientMessage: write");
        rfbCloseClient(cl);
        return;
    }

    cl->state = RFB_INITIALISATION;
}
