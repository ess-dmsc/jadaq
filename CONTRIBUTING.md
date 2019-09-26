# Contributing

The workflow for contributing to this repository is based on the GitHub
web interface.

## Create a github issue
Provide a one-liner text describing the new functionality or the improvement

This will give you a issue number, NNN.

    https://github.com/ess-dmsc/jadaq/issues/new

## Create a branch
Preferably from master. If you branch from another branch you might
want to mention from which one. The branch should be named issue_NNN
as a minimum. Adding a brief description is considered courteous.

    > git checkout master
    > git branch issue_NNN  # this is ok
    > git branch issue_NNN_update_readme # but this is better

## Edit
Make your edits, run your tests, add, commit and push to your heart's content.

    > git add README.md CONTRIBUTING.md
    > git commit -m 'elaborating on workflow'
    > git push --set-upstream origin issue_NNN_update_readme

## Create Pull-Request (PR)
Only admins are allowed to merge to master without a previous PR. The PR triggers a
code review requiring at least one reviewer approval before merging is allowed.
Additionally it is required that the branch builds successfully on jenkins.

Select which branch to compare with, typically master, and which branch contains the new changes.
Then select reviewers.

## Merge and delete
When the reviews and other checks have passed successfully you can merge the PR. After
that you should delete the branch.
