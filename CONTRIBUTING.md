# Contributing to noobWarrior
First things first, we suggest getting familiar with industry standard development tools (Git, CMake) and programming languages like C++ and Lua if you want to contribute technical changes to the project. As this is a software project, we prioritize programming first and foremost over any other talents.

We are all unpaid volunteers and we are doing this for fun, so you shouldn't really come to this with the expectation of money. (However, we may set up donations soon in the form of GitHub Sponsors so you can earn money as an active maintainer. Don't expect this to happen with 100% certainty though.)

If you possess other talents like art, you're still welcome to create artworks for specific parts of the project that need it.

## How do I know what to contribute??? I'm so overwhelmed!!!
The best developers eat their own dogfood. So grab a copy of noobWarrior for yourself and see what you think of it. What annoys you? What sets you off about it? What do you think should be there but is missing? Make mental notes in your head and see what you end up with.

Your list should first start off with the little things. Don't suddenly decide that you want to refactor the entire program and change the entire project structure for everybody.

After that, explore the source code and try to find where it makes most sense to place your feature.

If you're doing anything GUI related, look at QtGui. It's where all of the Qt GUI code is located. If it's anything internal-backend related on the C++ side, check Core. If you're modifying any Lua plugins (including the master server), they're located in AppDistribution.

Once you're experienced enough with the codebase, try looking at the issues on the GitHub page and take a crack at whatever you may find is easy.

## Read the documentation (and expand on it, if it's lacking)
The documentation provides guides on how the program is architected under the hood and it gives you a good introductory experience on how everything works in general. Read through as much of it as you can.

Think it's lacking? Go to AppDistribution/priv-plugins/docs/texts and improve upon it.

## How do I become a full-on maintainer?
Help make some small contributions, later express your desire to help maintain the project to other maintainers and they shall let you in to the secret Discord server. This server is not open to the public because Discord servers are usually terrible for fostering a community.

I will eventually have to make a more permanent solution, since closing off your development discussion to the public is not a good look.

## Policy on LLMs
I would be a hypocrite to ban LLMs from this project entirely, as I've used them before in some aspects for this project. That being said, please make sure any LLM contributions provide some value to the project, instead of serving as a way to fill up your GitHub commit history. Please actually know what you're doing and have some technical background in this instead of vibe-coding entire contributions. Look at the codebase, get yourself experienced with it, and write some code by hand before submitting any LLM-related PRs. That's all I ask for.

### Why did you use AI for some parts of the program??
I don't know everything. I am one guy who is essentially working on a project that emulates the entire backend of a billion dollar corporation, I don't have the funds to hire full-time maintainers for a hobbyist project that is basically unprofitable by nature, and I felt as if I needed to ship a prototype as quickly as possible because Roblox is not in a very good state right now. Using LLMs buys me time to do that at the cost of less maintainability. I think the value that this project provides to the Roblox community is enough to warrant using LLMs to speed up development.

That being said, there are real concerns with using AI like licensing issues that seem to be in a gray area, and the monopoly that frontier companies possess. If you're profusely anti-AI, you're "good enough" with C++, and have a decent understanding of how Roblox works in some capacity, then feel free to send PRs that replace LLM slop with human code. Search up the word "claude" in the repository to have a good understanding on what components are AI-assisted.

As the project progresses and more people are willing to contribute, I expect that we will slowly refactor the code to be more maintainable and less dependent on the works of those that aren't even living beings.

#### What parts were AI-assisted?
The Local RCC/offline Studio implementation, some of the API endpoints for the server emulator (like the toolbox and authentication), some code blocks in noobHook (the TLS cert redirection system and pinging system for instance), some unit tests, the file manager in the SDK (extends to the VFS implementation for databases), the avatar editor, and some other torturous boilerplate code that I could not be bothered to write by hand. Anything not listed here I either forgot to list or is hand-written by me.