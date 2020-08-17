<img width="180" align="left" style="float: left; margin: 0 10px 0 0;" alt="Shiro" src="https://i.imgur.com/NuDj76W.png">

# Shiro [![Circle CI](https://circleci.com/gh/Marc3842h/shiro/tree/master.svg?style=svg&circle-token=0fa5a9831ee745859ebeda892af66705bb0443b9)][0]

Shiro is a server-side implementation of the osu! cho protocol.
It delivers unmatched performance and features an accurate implementation
of the protocol.

It outperforms Ripple, Gigamons and even Bancho itself.
In terms of implementation accuracy, Shiro's implementation is
more accurate than both of Ripple and Gigamons.

Unlike other implementations, Shiro does not separate
Bancho and the Score Submission server.

## Features

Shiro handles all major aspects of a fully
functioning osu! server, which includes:

* Client login
* IRC-less chat & chat bot
* Leaderboards
* Multiplayer
* Online user listing with their presence
* osu!direct
* osu!tourney
* Rankings
* Replays
* Score submission
* Screenshots
* Spectating

## Documentation

Documentation such as installation and usage of Shiro
are available on the [Wiki][1].

## Docker [![Docker](https://img.shields.io/docker/pulls/marc3842h/shiro.svg?logo=docker)][2]

Shiro is available as ready-to-use Docker container, including its modules.
Please visit the [Wiki][3] for more information on how
to use the container.

## Contributing

We welcome all contributions to Shiro and its modules.

If you have found a issue with Shiro, feel free to [open a issue][4]
describing the issue in detail and how it occurred.

Code and documentation contributions can be made using
[Pull requests][5]. Please make sure you conform to the
general coding style of Shiro.

## License

Shiro is licensed under the GNU Affero General Public License,
version 3.0. Please visit the [LICENSE.txt][6] file for
the license and a summary of DOs & DON'Ts.

Third party libraries in the `src/thirdparty` may be
licensed under a different, but AGPL-3.0 compatible
license. Please visit the corresponding files and their
associated license for more information.

The branding `osu!` and `ppy` are protected by
trademark law. Fair use applies.

[0]: https://circleci.com/gh/Marc3842h/shiro
[1]: https://github.com/Marc3842h/shiro/wiki
[2]: https://hub.docker.com/r/marc3842h/shiro/
[3]: https://github.com/Marc3842h/shiro/wiki/Docker
[4]: https://github.com/Marc3842h/shiro/issues/new
[5]: https://github.com/Marc3842h/shiro/compare
[6]: https://github.com/Marc3842h/shiro/blob/master/LICENSE.txt
