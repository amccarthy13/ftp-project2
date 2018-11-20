Andrew Kiefer McCarthy, CNetID: 12161318

program name: pftp
Program downloads files from ftp servers.
required Flags: -s (hostname) -f (file name)
optional flags: -v | --version : returns application name, version number and author name.
                -h | --help : prints synopsis of application
                -p | --port : specifies port to be used for contacting ftp server
                -n | --username : specifies username for logging into server
                -P | --password : specifies password for logging into server
                -l | --log : specifies logfile for logging commands to and from ftp server. if logfile is set to "-"
                             all commands and server messages are printed to stdout
                -t | --thread : specifies config file which should contain the login, password, hostname, and absolute path to the file.
                     One can specify multiple servers for multi-threaded downloading. Note: if using a config file no hostname or file can
                     be specified in the options.  Any password or username specified in the options will be overwritten by the values
                     in the config file
                If no flags are passed the application synopsis is returned

Issues:

In my linux VM I had a recurring error with multithreading where one of my threads would fail to connect to the data socket.  This only happened in my VM with multithreading, and would only ever happen with one thread regardless of how many threads I was using to download.  I never got this problem on Mac OS 

Sometimes server and client command outputs contain random characters at the end or the beggining.  This is likely due to overflow from other buffers.  The buffers are still readable though
so it isn't a serious problem.
