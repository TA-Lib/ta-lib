!-------------------------------------------------------------------<Prolog>-
!   Name:       sflcvst.l
!   Title:      Dialog for 'convert string to time' function
!   Package:    Standard Function Library (SFL)
!
!   Written:    1996/01/05  iMatix SFL project team <sfl@imatix.com>
!   Revised:    1997/09/08
!
!   Copyright:  Copyright (c) 1996-2000 iMatix Corporation
!   License:    This is free software; you can redistribute it and/or modify
!               it under the terms of the SFL License Agreement as provided
!               in the file LICENSE.TXT.  This software is distributed in
!               the hope that it will be useful, but without any warranty.
!------------------------------------------------------------------</Prolog>-*/

After-Init:
    (--) Ok                                 -> Expect-Hour
          + Get-Next-Component

Expect-Hour:
    (--) Number                             -> Expect-Minute
          + Have-Hour
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Hour
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Minute:
    (--) Number                             -> Expect-Second
          + Have-Minute
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Minute
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Second:
    (--) Number                             -> Expect-Centisecond
          + Have-Second
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Second
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Centisecond:
    (--) Number                             -> Allow-Am-Pm
          + Have-Centisecond
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Centisecond
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Allow-Am-Pm:
    (--) Am-Pm                              -> Expect-Finished
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Finished:
    (--) Finished                           ->
          + Have-Complete-Time
          + Terminate-The-Program

Defaults:
    (--) Number                             ->
          + Have-Invalid-Time
          + Terminate-The-Program
    (--) Am-Pm                              ->
          + Have-Invalid-Time
          + Terminate-The-Program
    (--) Finished                           ->
          + Have-Complete-Time
          + Terminate-The-Program
    (--) Delimiter                          ->
          + Have-Delimiter
          + Get-Next-Component
    (--) Error                              ->
          + Have-Invalid-Time
          + Terminate-The-Program
