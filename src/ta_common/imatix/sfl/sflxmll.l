!-------------------------------------------------------------------<Prolog>-
!   Name:       sflxmll.l
!   Title:      Dialog for XML loader function
!   Package:    Standard Function Library (SFL)
!
!   Written:    1998/03/15  iMatix SFL project team <sfl@imatix.com>
!   Revised:    2000/01/20
!
!   Copyright:  Copyright (c) 1991-2000 iMatix Corporation
!   License:    This is free software; you can redistribute it and/or modify
!               it under the terms of the SFL License Agreement as provided
!               in the file LICENSE.TXT.  This software is distributed in
!               the hope that it will be useful, but without any warranty.
!------------------------------------------------------------------</Prolog>-*/

-source=sflxmll

After-Init:
    (--) File                               -> Getting-First-Item
          + Open-XML-File
          + Initialise-XML-Tree
          + Get-Content-Token
    (--) String                             -> Getting-First-Item
          + Initialise-XML-Tree
          + Get-Content-Token
    (--) Looks-Like-XML                     -> Examine-First-Item
          + Open-XML-File
          + Get-Content-Token
    (--) Error                              ->
          + Close-XML-File
          + Free-Partial-XML-Tree
          + Terminate-The-Program

Getting-First-Item:
    (--) Open                               -> Getting-Start-Tag
          + Expect-Name
          + Attach-New-Item
          + Note-Top-Level-Item
          + Get-Tag-Token
    (--) Processing                         ->
          + Skip-Rest-Of-Processing
          + Get-Content-Token
    (--) Comment                            ->
          + Skip-Rest-Of-Comment
          + Get-Content-Token
    (--) Ignore                             ->
          + Skip-Rest-Of-Section
          + Get-Content-Token
    (--) Space                              ->
          + Skip-Spaces    
          + Get-Content-Token
    (--) End-Of-File                        ->
          + Report-No-XML
          + Close-XML-File
          + Return-Error-Feedback
          + Terminate-The-Program

Getting-Start-Tag:
    (--) Name                               -> Getting-Start-Tag
          + Expect-Equals-Token
          + Expect-Literal
          + Attach-New-Attribute
          + Get-Tag-Token
    (--) Close                              -> Getting-Content
          + Get-Content-Token
    (--) Slash                              -> After-Closing-Item
          + Expect-Close
          + Close-The-Item
          + Check-For-Open-Item
    (--) Comment                            ->
          + Skip-Rest-Of-Comment
          + Get-Tag-Token

Getting-Content:
    (--) Open                               -> Getting-Start-Tag
          + Expect-Name
          + Attach-New-Item
          + Get-Tag-Token
    (--) Processing                         ->
          + Skip-Rest-Of-Processing
          + Get-Content-Token
    (--) Comment                            ->
          + Skip-Rest-Of-Comment
          + Get-Content-Token
    (--) Ignore                             ->
          + Skip-Rest-Of-Section
          + Get-Content-Token
    (--) Char                               ->
          + Collect-Item-Value
          + Get-Content-Token
    (--) Space                              ->
          + Collect-Item-Value
          + Get-Content-Token
    (--) Close                              -> After-Closing-Item
          + Expect-Name
          + Confirm-Item-Name
          + Remove-Values-If-All-White-Space
          + Close-The-Item
          + Expect-Close
          + Check-For-Open-Item

After-Closing-Item:
    (--) Ok                                 -> Getting-Content
          + Get-Content-Token
    (--) Optional-Items                     -> Getting-Optional-Tag
          + Get-Content-Token
    (--) No-More-Items
          + Expect-End-Of-File
          + Close-XML-File
          + Terminate-The-Program

Getting-Optional-Tag:
    (--) Open                               -> Getting-Start-Tag
          + Expect-Name
          + Attach-New-Item
          + Note-Top-Level-Item
          + Get-Tag-Token
    (--) Processing                         ->
          + Skip-Rest-Of-Processing
          + Get-Content-Token
    (--) Comment                            ->
          + Skip-Rest-Of-Comment
          + Get-Content-Token
    (--) Ignore                             ->
          + Skip-Rest-Of-Section
          + Get-Content-Token
    (--) Space                              ->
          + Skip-Spaces    
          + Get-Content-Token
    (--) End-Of-File                        ->
          + Close-XML-File
          + Terminate-The-Program

Examine-First-Item:
    (--) Processing                         ->
          + Close-XML-File
          + Terminate-The-Program
    (--) $other                             ->
          + Close-XML-File
          + Return-Error-Feedback
          + Terminate-The-Program

Defaults:
    (--) Error                              ->
          + Close-XML-File
          + Free-Partial-XML-Tree
          + Return-Error-Feedback
          + Terminate-The-Program
    (--) End-Of-File                        ->
          + Unexpected-Token-Error
    (--) $other                             ->
          + Unexpected-Token-Error
