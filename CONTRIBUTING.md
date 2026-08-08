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

## Policy on LLMs
I would be a hypocrite to ban LLMs from this project entirely, as I've used them before in some aspects for this project. That being said, please make sure any LLM contributions provide some value to the project, instead of serving as a way to fill up your GitHub commit history. Please actually know what you're doing and have some technical background in this instead of vibe-coding entire contributions. Look at the codebase, get yourself experienced with it, and write some code by hand before submitting any LLM-related PRs. That's all I ask for.