#! /bin/sh

ps -A |grep "concoord" | awk '{print $1}' | xargs kill
