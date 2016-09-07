# DarkCaverns
A basic roguelike built in C

## What's this all about?
The goal of this project is to build a classic roguelike game using the C language. A secondary goal is to do this with minimal dependence on pre-built libraries - we're going to be building most of the stuff that we need in our roguelike ourselves. We'll use the SDL library to give us cross-platform video support and input event handling, but we'll be building all the game-related code ourselves.

## Why C?
C is a great language for learning the nuts and bolts of what is happening on your computer. It respects you, the developer, by giving you total control over what is happening with your data and your code. Plus, it's a small language - and therefore relatively easy to keep in your head. 

Some people are frightened of C because it uses pointers and forces you to think about memory management, but it is my hope that at the end of this series, you will realize that all that stuff isn't as scary as is sounds, and by learning about it, you will become a better programmer. Even if you choose to do your next project in another language, you'll know a lot more about what's going on under the hood.

## Why aren't you using a library like libTCOD or something to make things easier?
While using a library such as libTCOD would certainly make some aspects of our game development easier, we end up trading control and understanding for ease of use. If we build all aspects of the game ourselves from scratch, we gain true understanding of how everything fits together, and that's the real point of this exercise.

Once you understand the underlying algorithms, they become less intimidating, and you'll hopefully gain the confidence to tweak the existing algorithms or create new ones to achieve unique effects in your own game.
