##!/usr/bin/perl
#
# Copyright Andreas Schlapbach, schlpbch@iam.unibe.ch, 2000
# http://iamexwiwww.unibe.ch/studenten/kfinger
#
# Touch at your own risk.

$mails   = '<A HREF="mailto:';
$mailsp	 = '">';
$maile   = '</A>';
$nlogins = '<FONT COLOR="#B0000">';
$logins  = '<FONT COLOR="#00A00">';
$blue	 = '<FONT COLOR="#82A7D0">';
$yellow  = '<FONT COLOR="#D0A000">';
$red	 = '<FONT COLOR="red">';
$close	 = '</FONT>';

# Those names get skipped, so if there's a user with such a name, bad luck.

@keywords=('Welcome','Directory','Login','Last','Mail','On','finger');
$keywordlist = join '|', @keywords;

$FINGERCMD   = "$ARGV[0]";  # The complete path to the finger cmd
$REFRESHRATE = "$ARGV[1]";  # The intervals in seconds until the page gets updated
$HOST        = "$ARGV[2]";  # host name
$USER        = "$ARGV[3]";  # user name

# HTML Header

printf <<HTMLHeader;
<HTML>
<HEAD>
  <meta http-equiv="refresh" content="$REFRESHRATE">
  <TITLE>finger $USER\@$HOST</TITLE>
  <style type="text/css">
  <!--  
  .nicetextbig {font-family: Verdana, Arial, Helvetica, sans-serif; 
	       font-size: 20px; 
	       font-weight: bold;
	       color: #999999}
  .nicetext    {font-family: Verdana, Arial, Helvetica, sans-serif; 
	       font-size: 12px; 
	       color: #999999}     
  .couriertext {font-family: Courier, sans-serif;
	       font-size: 12px; 
	       color: #FFFFCC}
  --> 
  </style>  
</HEAD>
<BODY BGCOLOR="#000000" TEXT="#DDDDDD" LINK="#82A7D0" ALINK="#999999" VLINK="#999999">
  <BR/>
  <BR/>
  <TABLE border="1" align="center" cellpadding="4" cellspacing="0">
  <TR> 
    <TH class="nicetextbig" align="middle" COLSPAN="2">
    <BR/>
    finger    $USER\@$HOST
    <BR/> 
    <BR/>       
  </TR>
  <TR>   
    <TH class="couriertext" align="center">
      <TABLE align="center" border="0" cellpadding="0" cellspacing="2">
HTMLHeader

# Run finger command and save it into a buffer

$buffer = `$FINGERCMD $USER\@$HOST`;
@lines = split /^/m, $buffer;

# Do highlighting using perl regular expressions on every line received.

foreach $output (@lines){
  $output =~ s/((\w)+\.(\w)+\.(\w){2,})/$yellow$1$close/gi;     # Highlight domain name  
  $output =~ s/((\d)+\.(\d)+\.(\d)+\.(\d)+)/$yellow$1$close/gi;   # Highlight IP number
  $output =~ s/(Linux)/$blue$1$close/gim;                       # Highlight Linux
  $output =~ s/Login:\s*(\w*)/Login: $mails$1\@$HOST$mailsp$1$maile/gi;
  $output =~ s/Login Name:\s*(\w*)/Login Name: $mails$1\@$HOST$mailsp$1$maile/gi;
  $output =~ s/Name:(((\s*)(\w+))+\n)/Name: $blue$1$close\n/gi;  # Linux
  $output =~ s/In real life:(((\s*)(\w+))+\n)/In real life:$blue$1$close\n/gi;  # Solaris
  $output =~ s/Directory:((\s*)(\/(\w)+)+)/Directory:$yellow$1$close/gi;   # Highlight Directory
  $output =~ s/Shell:((\s*)(\/(\w)+)+)/Shell:$yellow$1$close/gi;           # Highlight Shell
  $output =~ s/(not presently logged)/$nlogins$1$close/gi;
  $output =~ s/con (\w*)/con $logins$1$close/gi;
  $output =~ s/(New mail)/$yellow$1$close/gi;
  $output =~ s/^(No mail.)/$red$1$close/gim;
  $output =~ s/^(No plan.)/$red$1$close/gim;
  $output =~ s/^(\w+)/$mails$1\@$HOST$mailsp$1$maile/m unless ($output =~ m/$keywordlist/m);  

  # line consists of white space only?
  if ($output =~ m/^(\w*)\n/gi) {
    print "        <TR><TD><PRE>  </PRE></TD></TR>\n";
  } else {
    print "        <TR><TD><PRE>$output</PRE></TD></TR>\n";
  }    
}

print "      </TABLE>\n";
print "    </TH>\n";

# Finger-Talk options

if ($USER) { # is $USER nil ?
print <<UserQuery; 
  </TR>
  <TR>
    <TH class="couriertext" align="center" colspan="2">
    <A HREF='finger://$USER\@$HOST'>finger</A>
    </TH>
  </TR>
UserQuery

} else {

print <<HostQueryHead;
    <TH class="couriertext" align="center">
      <TABLE align="center" border="0" cellpadding="0" cellspacing="2">
HostQueryHead

    @lines = split /^/m, $buffer;
    foreach $output2 (@lines) {
    if ($output2 =~ m/^(\w+)/gi and not ($output2 =~ m/$keywordlist/m)) {
      $USER = $&; 
      print "        <TR><TD><PRE><A HREF='finger://$USER\@$HOST'>finger</A>\n</PRE></TD></TR>\n";
      # - <A HREF='talk://$USER\@$HOST'>talk</A>\n</PRE></TD></TR>\n";
    } else {      
      print "        <TR><TD><PRE>  </PRE></TD></TR>\n";  	
    }
}    

print <<HostQueryTail;
      </TABLE> 
    </TH>
  </TR>
HostQueryTail
}

# HTMLTail
printf <<HTMLTail;
  <TR>
    <TH class="nicetext">refresh rate: $REFRESHRATE seconds.</TH>
  </TR>
</TABLE>
</BODY>
</HTML>
HTMLTail


