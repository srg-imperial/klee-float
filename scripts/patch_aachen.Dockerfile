FROM comsys/klee-dev-fpbench-prebuilt:latest
MAINTAINER Dan Liew <daniel.liew@imperial.ac.uk>

ARG host_user_id=1000

# Need to be root to make these changes
USER root

# Change the UID of the user "user" to have the specified UID.
# The usermod  manual claims that it will fix permissions of
# the user's home directory and its contents automatically
RUN usermod --uid ${host_user_id} user

# Switch back to "user"
USER user
