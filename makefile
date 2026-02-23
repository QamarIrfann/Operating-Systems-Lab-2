# SOFE3950U - Operating Systems
# Lab 2: A Simple Shell (myshell)
# Name: Qamar Irfan
# Student #: (fill-in)
# Group: (fill-in)

myshell: myshell.c utility.c myshell.h
	gcc -Wall myshell.c utility.c -o myshell
