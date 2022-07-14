# Administrative "Access Control" Solution

MayI is a very vulnerable web app created to showcase bad JS and MongoDB coding practices.

## Installation

Note: The current build (v1.0) is sloppy and does not contain an automated way of doing the required tasks. This will be fixed eventually.

MayI uses npm to run, and requires a few things to properly work. First, we install the packages required if not already installed:

```bash
apt-get install mongodb
apt-get install npm
```
Also, make sure openssh-server is installed:
```bash
apt-get install openssh-server
```

Next, we need a new user named "MayIAgent" and a new group titled "MayIAccounts". The password can be anything you want:
```bash
groupadd MayIAccounts
useradd MayIAgent
usermod -G MayIAgent,MayIAccounts
```
Next, create a directory in "/" named "keys". Create another directory in "/" named "MayI". Ensure both directories are owned by MayIAgent:
```bash
mkdir /keys
mkdir /MayI
chown MayIAgent:MayIAgent /keys
chown MayIAgent:MayIAgent /MayI
```
Switch to the MayIAgent user and run ssh-keygen. Make sure that you don't password protect it, and place it into the /keys directory as AgentKey:
```bash
su MayIAgent
ssh-keygen
mv ~/.ssh/id_rsa /keys/AgentKey
```
Now move createUser.sh to /MayI: 
```bash
cp [repo_directory]/createUser.sh /MayI/createUser.sh
```
Once all of that is done, ensure sshd is running and you should be good to go! Dive into the repo directory until you get to the Node.js files, then either run
```bash
npm run dev
```
for more detailed logging or 
```bash
npm run prod
```
for a much more cleaner look. The webapp should start on port 4000! Don't forget to "register" the admin account with the email of "admin@mayi.com" and employee ID as "1".

## Hints
1. Always inspect the data brought to you by the app (index.html, login.html, etc)
2. How do you authenticate? What kind of sessions cookies are used?
3. ******* injection
4. OWASP Top 10 (2021) is a really good thing to know
5. GTFObins should always be in our browser bookmarks...

## Walkthroughs
There are a TON of ways to exploit this webapp, walkthroughs will be added below once written.
