#!/bin/bash

# get current branch
current_branch=$(git branch | sed -n -e 's/^\* \(.*\)/\1/p')

# git pull
git pull origin "$current_branch"
echo "====pull changes from '$current_branch' branch"

# git push
git push origin "$current_branch"
echo "====pushed changes to '$current_branch' branch"
