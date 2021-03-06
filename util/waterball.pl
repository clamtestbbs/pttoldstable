#!/usr/bin/perl
# $Id$
use lib '/home/bbs/bin/';
use LocalVars;
use Time::Local;
use POSIX;
use FileHandle;
use strict;
use Mail::Sender;

my($fndes, $fnsrc, $userid, $mailto, $outmode);
foreach $fndes ( <$JOBSPOOL/water.des.*> ){ #des: userid, mailto, outmode
    (open FH, "< $fndes") or next;
    chomp($userid = <FH>);
    chomp($mailto = <FH>);
    chomp($outmode= <FH>);
    close FH;
    next if( !$userid );
    print "$userid, $mailto, $outmode\n";
    `rm -Rf $TMP/water`;
    `mkdir -p $TMP/water`;

    $fnsrc = $fndes;
    $fnsrc =~ s/\.des\./\.src\./;
    eval{
	process($fnsrc, "$TMP/water/", $outmode, $userid);
    };
    if( $@ ){
	print "$@\n";
    }
    else{
	chdir "$TMP/water";
	if( $mailto eq '.' || $mailto =~ /\.bbs/ ){
	    $mailto = "$userid.bbs\@$hostname" if( $mailto eq '.' );
	    foreach my $fn ( <$TMP/water/*> ){
		my $who = substr($fn, rindex($fn, '/') + 1);
		my $content = '';
		open FH, "< $fn";while( <FH> ){chomp;$content .= "$_\n";}
		if( !MakeMail({mailto  => $mailto,
			       subject => "和 $who 的水球記錄",
			       body    => $content,
			   }) ){ print "fault\n"; }
		sleep(2) if( $mailto =~ /\.bbs/ );
	    }
	    unlink $fnsrc;
	    unlink $fndes;
	}
	else{
	    my $body = 
		"親愛的使用者您好:\n\n".
		"歡迎您使用 Ptt系列的水球整理功能 ^_^\n".
		"水球整理的結果被壓縮好附加在本信中\n".
		"您須要先將其解壓縮 (如用 tar+gunzip, winzip 等程式)\n".
		"解出來的檔案為純文字格式, \n".
		"您可以透過任何純文字編輯程式 (如 emacs, notepad, word)\n".
		"打開它進行編輯整理\n\n".
		"再次感謝您使用本系統以及對 $hostname 的支持 ^^\n".
		"\n $hostname 站長群 ". POSIX::ctime(time());
	    if( MakeMail({tartarget => "$TMP/$userid.waterball.tgz",
			  tarsource => "*",
			  mailto    => "$userid <$mailto>",
			  subject   => "水球紀錄",
			  body      => $body}) ){
		unlink $fnsrc;
		unlink $fndes;
	    }
	}
    }
}

sub process
{
    my($fn, $outdir, $outmode, $me) = @_;
    my($cmode, $who, $time, $say, $orig, %FH, %LAST, $len);
    open DIN, "< $fn";
    while( <DIN> ){
	chomp;
	next if( !(($cmode, $who, $time, $say, $orig) = parse($_)) );
	next if( !$who );

	if( ! $FH{$who} ){
	    $FH{$who} = new FileHandle "> $outdir/$who";
	}
	if( $outmode == 0 ){
	    next if( $say =~ /<<下站通知>> -- 我走囉！/ ||
		     $say =~ /<<上站通知>> -- 我來啦！/    );
	    if( $time - $LAST{$who} > 1800 ){
		if( $LAST{$who} != 0 ){
		    ($FH{$who})->print( POSIX::ctime($LAST{$who}) , "\n");
		}
		($FH{$who})->print( POSIX::ctime($time) );
		$LAST{$who} = $time;
	    }
	    $len = (length($who) > length($me) ? length($who) : length($me))+1;
	    ($FH{$who})->printf("%-${len}s %s\n", ($cmode?$who:$me).':', $say);
	}
	elsif( $outmode == 1 ){
	    ($FH{$who})->print("$orig\n");
	}
    }
    if( $outmode == 0 ){
	foreach( keys %FH ){
	    ($FH{$_})->print( POSIX::ctime($LAST{$_}) );
	}
    }
    foreach( keys %FH ){
	($FH{$_})->close();
    }
    close DIN;
}

sub parse
{
    my $dat = $_[0];
    my($cmode, $who, $year, $month, $day, $hour, $min, $sec, $say);
    if( $dat =~ /^To/ ){
	$cmode = 0;
	($who, $say, $month, $day, $year, $hour, $min, $sec) =
	    $dat =~ m|^To (\w+):\s*(.*)\[(\d+)/(\d+)/(\d+) (\d+):(\d+):(\d+)\]|;
    }
    else{
	$cmode = 1;
	($who, $say, $month, $day, $year, $hour, $min, $sec) =
	    $dat =~ m|★(\w+?)\[37;45m\s*(.*).*?\[(\w+)/(\w+)/(\w+) (\w+):(\w+):(\w+)\]|;

    }
#    $time = timelocal($sec,$min,$hours,$mday,$mon,$year);

    return undef if( $month == 0 );
    return ($cmode, $who, timelocal($sec, $min, $hour, $day, $month - 1, $year), $say, $_[0]);
}

sub MakeMail
{
    my($arg) = @_;
    my $sender;
    `$TAR zcf $arg->{tartarget} $arg->{tarsource}`
	if( $arg->{tarsource} );
    $sender = new Mail::Sender{smtp => $SMTPSERVER,
			       from => "$hostname水球整理程式 <$userid.bbs\@$MYHOSTNAME>"};
    foreach( 0..3 ){
	if( (!$arg->{tartarget} &&
	     $sender->MailMsg({to      => $arg->{mailto},
			       subject => $arg->{subject},
			       msg     => $arg->{body}
			   }) ) ||
	    ($arg->{tartarget} && 
	     $sender->MailFile({to      => $arg->{mailto},
				subject => $arg->{subject},
				msg     => $arg->{body},
				file    => $arg->{tartarget}})) ){
		unlink $arg->{tartarget} if( $arg->{tartarget} );
		return 1;
	    }
    }
    $sender->MailMsg({to      => "$userid.bbs\@$MYHOSTNAME",
		      subject => "無法寄出水球整理",
		      msg     =>
			  "親愛的使用者您好\n\n".
			  "你的水球整理記錄無法寄達指定位置 $mailto \n\n".
			  "$hostname站長群 敬上 ".POSIX::ctime(time())});
    unlink $arg->{tartarget} if( $arg->{tartarget} );
    return 1;
}
