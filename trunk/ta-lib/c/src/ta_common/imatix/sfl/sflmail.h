/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmail.h
    Title:      SMTP mailer function
    Package:    standard function library (sfl)

    Written:    06/18/97 Scott Beasley (jscottb@infoave.com)
    Revised:    1999/07/06

    Synopsis:   Functions to format and send SMTP messages.  Messages
                can contain attachments, and be sent with "cc"'s "bcc"'s as
                well as the normal "to" receivers.
            
    Copyright:  Copyright (C) 1991-2000 Scott Beasley and iMatix Corporation
    License:    this is free software; you can redistribute it and/or modify
                it under the terms of the sfl license agreement as provided
                in the file license.txt.  this software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _sflmail_included               /*  allow multiple inclusions        */
#define _sflmail_included

typedef struct SMTP {
   char *strSmtpServer;
   char *strMessageBody;
   char *strSubject;
   char *strSenderUserId;
   char *strFullSenderUserId;          /* to be filled with: "realname" <e-mail> */
   char *strDestUserIds;
   char *strFullDestUserIds;           /* to be filled with: "realname" <e-mail> */   
   char *strCcUserIds;
   char *strFullCcUserIds;             /* to be filled with: "realname" <e-mail> */
   char *strBccUserIds;
   char *strFullBccUserIds;
   char *strRetPathUserId;
   char *strRrcpUserId;
   char *strMsgComment;
   char *strMailerName;
   char *strBinFiles;
   char *strTxtFiles;
   char strlast_smtp_message[513];
   int  debug;
   char *strDebugFile;
   int  mime;
   int  encode_type;
   int  connect_retry_cnt;
   int  retry_wait_time;
} SMTP;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif
int smtp_send_mail_ex (SMTP *smtp);
int smtp_send_mail    (char *strSmtpServer,       char *strMessageBody,
                       char *strSubject,          char *strSenderUserId,
                       char *strFullSenderUserId, char *strDestUserIds,
                       char *strFullDestUserIds,  char *strCcUserIds,
                       char *strFullCcUserIds,    char *strBccUserIds,
                       char *strFullBccUserIds,   char *strRetPathUserId,
                       char *strRrcpUserId,       char *strMsgComment,
                       char *strMailerName,       char *strBinFiles,
                       char *strTxtFiles,         char *strDebugFile );
#ifdef __cplusplus
}
#endif

#endif

