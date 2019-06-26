# General Guidelines

## Branch setup
- ``master`` : Release branch, only stable code from the ``devel`` branch to merged into here.
- ``devel`` : Development staging branch, all feature branches are to be merged in here for testing before getting merged into ``master``.

Developers shall commit their code to a new branch, which is feature-oriented i.e. that different features should be developed in seperate branches. Every branch should have an open Issue or Merge Request once it's pushed to the main repository.

## Single or Occasional Commits
If you expect to only commit code occasionaly or a single time the best way to contribute is to fork the repository and submit a merge request from your personal repository to the ``devel`` branch of the YARR repository.

## Full Developer or Frequent Commits
If you would like to become a full developer or expect to perform more frequent commits please subscribe to the [yarr-devel](https://e-groups.cern.ch/e-groups/EgroupsSubscription.do?egroupName=yarr-devel) mailing list. Once approved you can push your development branch to the YARR repository and once ready create a merge request into the ``devel`` branch.

# Coding Style

No specific style is enforced, that being said if you edit existing code adhere to existing coding style.

Soon enough we will add a style checker.
