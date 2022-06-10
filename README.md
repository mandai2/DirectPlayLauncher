# DirectPlayLauncher
Command-line launcher for DirectPlay games

In my searching I have found some other examples of this but they often have unwanted features or the functions are split up which make it difficult for anyone not familiar with the API to understand what the code is actually doing.

Example usage:

DirectPlayLauncher.exe {95883290-2B70-459A-A5EE-7957E5991902} mysession player Host

or

DirectPlayLauncher.exe {95883290-2B70-459A-A5EE-7957E5991902} mysession player2 Join 192.168.1.13

Note the arguments are case-sensitive.
Sometimes joining by host name is more reliable than IP address (possible DirectPlay bug?).


Additionally I would like to find out how the host can launch to restore a saved game, but so far I haven't seen that done in any other examples. If anyone has any suggestions or corrections to this code please make a post and I'll gladly look into it.


Tested on Windows XP 32-bit, Windows 7 64-bit, Windows 10 64-bit. It should also work on other versions.

Compiled with Microsoft Visual Studio 2005, using the DirectX SDK (Aug 2007).
Note DirectPlay has been depreciated for a long time so the needed headers/libraries might not be included in the newer versions of the SDK.
