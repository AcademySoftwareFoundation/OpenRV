#!/usr/bin/env perl
#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
#
# SPDX-License-Identifier: Apache-2.0 
#

#
#   Perl seems to handle this pretty well
#

sub ack
{
    $m = shift( @_ );
    $n = shift( @_ );
    if( $m == 0 )
    {
        return $n + 1;
    }
    elsif( $n == 0 )
    {
        return ack($m-1, 1);
    }
    else
    {
        return ack($m-1, ack($m, $n-1));
    }
}

$k = 10;
ack( 3, $k );

