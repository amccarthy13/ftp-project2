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

When doing multi-threaded download, sometimes the downloaded file is broken and can't be open.  I wasn't able to pinpoint what was causing this, but it seems to be issues with the read buffers
and them sometimes being corrupted.  I don't know if this was an issue with my code or an issue caused by my OS.  To guarantee the file is downloaded correctly, ideally the program should be run
at least 10 times until a non-broken file is received.  This problem doesn't happen when using a single thread to download files (with or without using a config file).

Sometimes server and client command outputs contain random characters at the end or the beggining.  This is likely due to overflow from other buffers.  The buffers are still readable though
so it isn't a serious problem.