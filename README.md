## TODO

- [ ] Get backup (keypresses) and save working
- [ ] `u` and `r` keystroke
- [ ] Get Git gutters working
- [ ] Soft wrap choice
- [ ] Support highlighting ticks in JavaScript
- [ ] Load config into memory instead of directly working with it + add support for updating config
- [ ] Get tree view similar to Vim's ":Ex" working, starting in the directory of the given file or just the default directory if one isn't provided
- [ ] Be able to move around and open files through the tree view
- [ ] Get tab system working, `;` and Ctrl+number to move between tabs

## About

Text editor written from scratch using the [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) tutorial that I daily dogfood. Wanted to make it my own (in fact, this was written with `marrow`!), so I've added/am adding some extra, opinionated features. Libraries currently being used include:

* [`inih`](https://github.com/benhoyt/inih), for parsing INI files
* [`tiny-regex-c`](https://github.com/kokke/tiny-regex-c), for parsing regex strings

Although I would love to write my own in the future.

## Design decisions

Over the next couple months, I'll be working on adding the following, which are obviously all quite opinionated:

- [X] Different modes like Vim.
- [X] Configuration file in `config.h`, with options for features like line numbers, Git gutters, and WakaTime integration (all things I use in my Vim config) 
- [X] Syntax highlighting for other programming languages besides C. The way the guide does syntax highlighting is quite frankly horrendous, but I can't think of a quicker way to do it without generating tokens for each language to categorize each term. I'll start by modularizing the syntax highlighting code and then adding a couple of languages that I use often, like Python and JavaScript.
- [X] A tree viewer. This should be relatively easy using `dirent.h` (not supported in Windows though) and adapting the code currently being used to track keystrokes to track moving through the file tree.
- [X] Tab system. Split view can come later, if I'm so inclined.
- [ ] Terminal emulator? Wrapper around `libvterm`, except I don't know how to get started.
- [ ] Command system with a modal (like the command dialog in VSCode) with fuzzy searching across files, which should make moving around files a lot easier.
- [ ] Autocomplete system.
- [ ] Figure out a better plugin system so this is actually extensible (and usable by other people).
- [ ] Optimizing the code. Ran it through Valgrind so there aren't any memory issues, but some parts of the code could definitely be written better.
- [ ] Potentially writing all the libraries I use from scratch (I know, I know).
- [ ] Testing across multiple OSes/architectures.

These are all features I use in Vim/VSCode on a daily basis and thought would be useful to add on. 

## Compatibility

Want to try this out? Great. I've only run this on my Macbook and Ubuntu on an EC2 instance. On Windows, definitely sure this can't run, although I might patch issues in the future if I ever need to work on Windows.

## Wait, so are you actually using this?

Yes! Just mentioned it before, but I wrote this README with `marrow`. I simply set up an alias for `zsh` so I can run it with `marrow <file>`.

Does this mean I'm using it solely by itself? Nope. This was intended to replace Vim in my Vim + VSCode setup, although I think I'll still be using all three. (This for funsies, Vim to learn Vim, and VSCode when I need to switch between multiple files every few seconds). It's a lot of fun using something you built!