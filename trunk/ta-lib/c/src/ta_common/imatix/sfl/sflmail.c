/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmail.c
    Title:      SMTP mailer functions
    Package:    standard function library (sfl)

    Written:    1997/06/18  Scott Beasley <jscottb@infoave.com>
    Revised:    2000/01/19  iMatix SFL project team <sfl@imatix.com>

    Synopsis:   Functions to format and send SMTP messages.  Messages
                can contain attachments, and be sent with "cc"'s "bcc"'s as
                well as the normal "to" receivers.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    this is free software; you can redistribute it and/or modify
                it under the terms of the sfl license agreement as provided
                in the file license.txt.  this software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"
#include "sflsock.h"
#include "sfldate.h"
#include "sflmime.h"
#include "sflprint.h"                   /*  snprintf functions               */
#include "sflmail.h"

/*  Macros & defines                                                         */
/* Macro to encoding a char and make it printable. */
#define ENC(c) ((c) ? ((c) & 077) + ' ': '`')
/* Macro to write too the smtp port. */
#define smtp_send_data(sock,strout) write_TCP((sock),(strout),strlen((strout)))

/* Stactic function prototypes                                               */
static int uuencode      (char *strin, char *strout, char *last_smtp_message);
static void putgroup     (register char *strgroup, register FILE *fp);
static int getreply      (int iSocket, SMTP *smtp);
static char *getfilename (char *strfullpath);


/*  ---------------------------------------------------------------------[<]-
    Function: smtp_send_mail_ex

    Synopsis: Format and send a SMTP message.  This function gives you the
    options of sneding to multi receivers, CC's, Bcc's and also send
    UUencoded attachments. Receivers and files are ";" or "," terminated.
    NOTE: The sock_init function should be called before use of this
    function.
    ---------------------------------------------------------------------[>]-*/

int smtp_send_mail_ex (
   SMTP *smtp)
{
   FILE *fpin;
   int iCnt;
   sock_t iSocket;
   char strOut[514], strFile[256], strRetBuff[513];
   char strUUEFile[256], *strRcptUserIds;
   int iOld_ip_nonblock = ip_nonblock, rcptUserIdsLen;

   /*  Make sure we block on socket accesses                                 */
   ip_nonblock = FALSE;

   /* Open up the SMTP port (25 most of the time). */
   iSocket = connect_socket (smtp->strSmtpServer,
                             "smtp", "tcp", NULL,
                             smtp->connect_retry_cnt,
                             smtp->retry_wait_time);

   if (getreply (iSocket, smtp) > 400 || iSocket < 0)
       return -1;

   /* Format a SMTP meassage header.  */
   /* Just say hello to the mail server. */
   xstrcpy (strOut, "HELO ", get_hostname (), "\n", NULL);
   smtp_send_data (iSocket, strOut);
   if (getreply (iSocket, smtp) > 400)
       return -2;

   /* Tell the mail server who the message is from. */
   xstrcpy (strOut, "MAIL FROM:<", smtp->strSenderUserId, ">\n", NULL);
   smtp_send_data (iSocket, strOut);
   if (getreply (iSocket, smtp) > 400)
       return -3;

   rcptUserIdsLen = (strlen (smtp->strDestUserIds) + 
                     strlen (smtp->strCcUserIds) +
		     strlen (smtp->strBccUserIds) + 3);

   strRcptUserIds = (char *) malloc (rcptUserIdsLen);
   snprintf (strRcptUserIds, rcptUserIdsLen, "%s;%s;%s", 
	     smtp->strDestUserIds, smtp->strCcUserIds, smtp->strBccUserIds);

   /* The following tells the mail server who to send it to. */
   iCnt = 0;
   while (1)
     {
       getstrfld (strRcptUserIds, iCnt++, 0, ",;", strRetBuff);

       if (*strRetBuff)
         {
           xstrcpy (strOut, "RCPT TO:<", strRetBuff, ">\r\n", NULL);
           smtp_send_data (iSocket, strOut);
           if (getreply (iSocket, smtp) > 400)
               return -4;
         }

       else
           break;
     }

   free (strRcptUserIds);

   /* Now give it the Subject and the message to send. */
   smtp_send_data (iSocket, "DATA\r\n");
   if (getreply (iSocket, smtp) > 400)
       return -5;

   /* Set the date and time of the message. */
   xstrcpy ( strOut, "Date: ", encode_mime_time (date_now (), time_now ()),
             " \r\n", NULL );

   /* The following shows all who it was sent to. */
   if ( smtp->strFullDestUserIds && *smtp->strFullDestUserIds )
    {
       replacechrswith (smtp->strFullDestUserIds, ";", ',');
       xstrcpy (strOut, "To: ", smtp->strFullDestUserIds, "\r\n", NULL);
     }
   else
    {
       replacechrswith (smtp->strDestUserIds, ";", ',');
       xstrcpy (strOut, "To: ", smtp->strDestUserIds, "\r\n", NULL);
    }

   /* Set up the Reply-To path. */
   if (!smtp->strRetPathUserId || !*smtp->strRetPathUserId)
       smtp->strRetPathUserId = smtp->strSenderUserId;

   if ( strstr( smtp->strRetPathUserId, "<" ) != NULL &&
        strstr( smtp->strRetPathUserId, ">" ) != NULL )
     {
       xstrcat (strOut, "Reply-To:", smtp->strRetPathUserId, "\r\n", NULL);
     }
   else
     {
       xstrcat (strOut, "Reply-To:<", smtp->strRetPathUserId, ">\r\n", NULL);
     }

   if ( smtp->strFullSenderUserId && *smtp->strFullSenderUserId )
     {
       xstrcat (strOut, "Sender:", smtp->strFullSenderUserId, "\r\n", NULL);
       xstrcat (strOut, "From:", smtp->strFullSenderUserId, "\r\n", NULL);
     }
   else
     {
       xstrcat (strOut, "Sender:", smtp->strSenderUserId, "\r\n", NULL);
       xstrcat (strOut, "From:", smtp->strSenderUserId, "\r\n", NULL);
     }
   smtp_send_data (iSocket, strOut);

   *strOut = '\0';

   /* Post any CC's. */
   if (smtp->strFullCcUserIds && *smtp->strFullCcUserIds)
     {
       replacechrswith (smtp->strFullCcUserIds, ";", ',');
       xstrcat (strOut, "Cc:", smtp->strFullCcUserIds, "\r\n", NULL );
     }
   else
   if (smtp->strCcUserIds && *smtp->strCcUserIds)
     {
       replacechrswith (smtp->strCcUserIds, ";", ',');
       xstrcat (strOut, "Cc:", smtp->strCcUserIds, "\r\n", NULL );
     }

   /* Post any BCC's. */
   if (smtp->strFullBccUserIds && *smtp->strFullBccUserIds)
     {
       replacechrswith (smtp->strFullBccUserIds, ";", ',');
       xstrcat (strOut, "Bcc:", smtp->strFullBccUserIds, "\r\n", NULL);
     }
   else
   if (smtp->strBccUserIds && *smtp->strBccUserIds)
     {
       replacechrswith (smtp->strBccUserIds, ";", ',');
       xstrcat (strOut, "Bcc:", smtp->strBccUserIds, "\r\n", NULL);
     }
   /* Post any Return-Receipt-To. */
   if (smtp->strRrcpUserId && *smtp->strRrcpUserId)
       xstrcat (strOut, "Return-Receipt-To:", smtp->strRrcpUserId, ">\r\n",
                NULL);

   if (smtp->strMailerName && *smtp->strMailerName)
       xstrcat (strOut, "X-Mailer: ", smtp->strMailerName, "\r\n", NULL);
   else
       strcat (strOut, "X-Mailer: sflmail function\r\n");

   /* Set the mime version. */
   strcat (strOut, "MIME-Version: 1.0\r\n");
   strcat (strOut,
   "Content-Type: Multipart/Mixed; boundary=Message-Boundary-21132\r\n");

   smtp_send_data (iSocket, strOut);

   /* Write out any message comment included. */
   xstrcpy (strOut, "Comments: ", smtp->strMsgComment, "\r\n", NULL);

   /* Send the subject and message body. */
   xstrcat (strOut, "Subject:", smtp->strSubject, "\n\r\n", NULL);
   smtp_send_data (iSocket, strOut);

   /* Keep rfc822 in mind with all the sections. */
   if (smtp->strMessageBody && *smtp->strMessageBody)
     {
       strcat (strOut, "\r\n--Message-Boundary-21132\r\n");
       strcat (strOut, "Content-Type: text/plain; charset=US-ASCII\r\n");
       strcat (strOut, "Content-Transfer-Encoding: 7BIT\r\n");
       strcat (strOut, "Content-description: Body of message\r\n\r\n");
       smtp_send_data (iSocket, strOut);
       smtp_send_data (iSocket, smtp->strMessageBody);
       smtp_send_data (iSocket, "\r\n");
     }

   /* Include any Text type files and Attach them to the message. */
   if (smtp->strTxtFiles && *smtp->strTxtFiles)
     {
       iCnt = 0;
       while (1)
         {
           getstrfld (smtp->strTxtFiles, iCnt++, 0, ",;", strFile);
           trim (strFile);
           if (*strFile)
             {
               fpin = fopen (strFile, "rb");
               if (!fpin)
                 {
                   strcpy (smtp->strlast_smtp_message, strFile);
                   return -6;
                 }

               strcpy (strOut, "\r\n--Message-Boundary-21132\r\n");
               strcat (strOut,
                       "Content-Type: text/plain; charset=US-ASCII\r\n");
               strcat (strOut, "Content-Transfer-Encoding: 7BIT\r\n");
               xstrcat (strOut, "Content-Disposition: attachment; filename=",
                        getfilename (strFile), "\r\n\n", NULL);
               smtp_send_data (iSocket, strOut);
               while (!feof (fpin))
                 {
                   memset (strRetBuff, 0, 513);
                   fread (strRetBuff, sizeof (char), 512, fpin);
                   smtp_send_data (iSocket, strRetBuff);
                 }

               fclose (fpin);
             }
           else
               break;
         }
     }

   /* Attach any bin files to the message. */
   if (smtp->strBinFiles && *smtp->strBinFiles)
     {
       iCnt = 0;
       while (1)
         {
           getstrfld (smtp->strBinFiles, iCnt++, 0, ",;", strFile);
           trim (strFile);
           if (*strFile)
             {
               strcpy (strUUEFile, strFile);
               if (strchr (strUUEFile, '.'))
                   *((strchr (strUUEFile, '.')))= (char)NULL;
               strcat (strUUEFile, ".uue");
               uuencode (strFile, strUUEFile, smtp->strlast_smtp_message);
               fpin = fopen (strUUEFile, "rb");
               if (!fpin)
                 {
                   return -6;
                 }

               strcpy (strOut, "\r\n--Message-Boundary-21132\r\n");
               xstrcat (strOut,
                        "Content-Type: application/octet-stream; name=",
               getfilename (strFile), "\r\n", NULL);
               strcat (strOut, "Content-Transfer-Encoding: x-uuencode\r\n");
               xstrcat (strOut, "Content-Disposition: attachment; filename=",
                        getfilename (strFile), "\r\n\n", NULL);
               smtp_send_data (iSocket, strOut);
               while (!feof (fpin))
                 {
                   memset (strRetBuff, 0, 513);
                   fread (strRetBuff, sizeof (char), 512, fpin);
                   smtp_send_data (iSocket, strRetBuff);
                 }

               fclose (fpin);

               if ( !smtp->debug )
                  unlink (strUUEFile);
             }
           else
               break;
         }
     }

   /* This ends the message. */
   smtp_send_data (iSocket, ".\r\n");
   if (getreply (iSocket, smtp) > 400)
        return -7;

   /* Now log off the SMTP port. */
   smtp_send_data (iSocket, "QUIT\n");
   if (getreply (iSocket, smtp) > 400)
        return -8;

   /*
      Clean-up.
   */
   /* Close the port up. */
   close_socket (iSocket);

   /* If a clean send, then reset and leave. */
   ip_nonblock = iOld_ip_nonblock;

   return 0;
}

/*  ---------------------------------------------------------------------[<]-
    Function: smtp_send_mail

    Synopsis: Format and send a SMTP message, by calling the
    smtp_send_mail_ex function.  This function is kept to be compatable
    with previous versions of smtp_send_mail, smtp_send_mail_ex should
    now be used, this will be deleted soon.
    ---------------------------------------------------------------------[>]-*/

int smtp_send_mail (
   char *strSmtpServer,
   char *strMessageBody,
   char *strSubject,
   char *strSenderUserId,
   char *strFullSenderUserId,
   char *strDestUserIds,
   char *strFullDestUserIds,
   char *strCcUserIds,
   char *strFullCcUserIds,
   char *strBccUserIds,
   char *strFullBccUserIds,
   char *strRetPathUserId,
   char *strRrcpUserId,
   char *strMsgComment,
   char *strMailerName,
   char *strBinFiles,
   char *strTxtFiles,
   char *strDebugFile )
{
   SMTP smtp;

   smtp.strSmtpServer = strSmtpServer;
   smtp.strMessageBody = strMessageBody;
   smtp.strSubject = strSubject;
   smtp.strSenderUserId = strSenderUserId;
   smtp.strFullSenderUserId = strFullSenderUserId;
   smtp.strDestUserIds = strDestUserIds;
   smtp.strFullDestUserIds = strFullDestUserIds;
   smtp.strCcUserIds = strCcUserIds;
   smtp.strFullCcUserIds = strFullCcUserIds;
   smtp.strBccUserIds = strBccUserIds;
   smtp.strFullBccUserIds = strFullBccUserIds;
   smtp.strRetPathUserId = strRetPathUserId;
   smtp.strRrcpUserId = strRrcpUserId;
   smtp.strMsgComment = strMsgComment;
   smtp.strMailerName = strMailerName;
   smtp.strBinFiles = strBinFiles;
   smtp.strTxtFiles = strTxtFiles;
   smtp.connect_retry_cnt = 3;
   smtp.retry_wait_time = 0;
   smtp.debug = 0;
   smtp.strDebugFile = strDebugFile;

   return smtp_send_mail_ex (&smtp);
}

/*
 *  uuencode -- internal
 *
 *  Synopsis: Uuencode a file, with the output going to a new file. This
 *  function is used by smtp_send_mail.
 * -------------------------------------------------------------------------*/

static int uuencode (
   char *strIn,
   char *strOut,
   char *strlast_smtp_message)
{
   char strLine[46];
   int iCnt, iLineLen;
   FILE *fpin, *fpout;

   if (!(fpin = fopen (strIn, "rb")))
     {
       strcpy (strlast_smtp_message, strIn);
       return 1;
     }

   if (!(fpout = fopen (strOut, "wb")))
     {
       strcpy (strlast_smtp_message, "Could not create temp file for write.");
       return 1;
     }

   fprintf (fpout, "begin 666 %s\n", getfilename (strIn));

   while (1)
     {
       iLineLen = fread (strLine, sizeof (char), 45, fpin);
       if (iLineLen <= 0)
           break;

       fputc (ENC (iLineLen), fpout);

       for (iCnt = 0; iCnt < iLineLen; iCnt += 3)
         {
           putgroup (&strLine[iCnt], fpout);
         }

       fputc ('\n', fpout);
     }

   fprintf (fpout, "end\n");

   fclose (fpin);
   fclose (fpout);
   return 0;
}

/*
 *  putgroup -- internal
 *
 *  Synopsis: Write out 3 char group to uuendcoded file making it
 *  printable  This function is used by uuencode.
 * -------------------------------------------------------------------------*/

static void putgroup (
   char *strgroup,
   FILE *fp)
{
    int ichr1, ichr2, ichr3, ichr4;

    ichr1 =   strgroup [0] >> 2;
    ichr2 = ((strgroup [0] << 4) & 0x030) | ((strgroup [1] >> 4) & 0x00f);
    ichr3 = ((strgroup [1] << 2) & 0x03c) | ((strgroup [2] >> 6) & 0x003);
    ichr4 =   strgroup [2] & 0x03f;

    fputc (ENC (ichr1), fp);
    fputc (ENC (ichr2), fp);
    fputc (ENC (ichr3), fp);
    fputc (ENC (ichr4), fp);
}

/*
 *  getreply -- internal
 *
 *  Synopsis: Get a reply from the SMTP server and see thats it's not
 *  an error. This function is used by smtp_send_mail.
 * -------------------------------------------------------------------------*/

static int getreply (
   int iSocket,
   SMTP *smtp)
{
    int read_size;
    FILE *fpout;
    char strRetBuff[513];

    read_size = read_TCP ((sock_t)iSocket, strRetBuff, 512);
    /* See if we have not gotten a responce back from the mail server. */
    if (read_size == 0)
        return 777;
    else
        strRetBuff [read_size] = '\0';

    /* Save off server reply. */
    strcpy (smtp-> strlast_smtp_message, strRetBuff);
    trim (strRetBuff);
    strRetBuff [3] = '\0';

    if ( smtp->debug )
      {
        if ((fpout = fopen (smtp->strDebugFile, "a")))
          {
            fputs (smtp->strlast_smtp_message, fpout );
            fclose (fpout);
          }
      }
   return atoi (strRetBuff);
}

/*
 *  getfilename -- internal
 *
 *  Synopsis: Get's the name from the full path of a file. This function
 *  is used by smtp_send_mail.
 * -------------------------------------------------------------------------*/

static char *getfilename (
   char *strFullPath)
{
   int iLen;
   char *strTmp;

   iLen = strlen (strFullPath);
   strTmp = (strFullPath + iLen);
   while (1)
     {
       if (*strTmp == PATHEND || !iLen)
           break;
       strTmp--;
       iLen--;
     }

   if (*strTmp == PATHEND)
       strTmp++;

   return strTmp;
}
