# Hera

[contributors-shield]: https://img.shields.io/github/contributors/Alessandro-Salerno/hera.svg?style=flat-square
[contributors-url]: https://github.com/Alessandro-Salerno/hera/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/Alessandro-Salerno/hera.svg?style=flat-square
[forks-url]: https://github.com/Alessandro-Salerno/hera/network/members
[stars-shield]: https://img.shields.io/github/stars/Alessandro-Salerno/hera.svg?style=flat-square
[stars-url]: https://github.com/Alessandro-Salerno/hera/stargazers
[issues-shield]: https://img.shields.io/github/issues/Alessandro-Salerno/hera.svg?style=flat-square
[issues-url]: https://github.com/Alessandro-Salerno/hera/issues
[license-shield]: https://img.shields.io/github/license/Alessandro-Salerno/hera.svg?style=flat-square
[license-url]: https://github.com/Alessandro-Salerno/hera/blob/master/LICENSE.txt

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
![](https://tokei.rs/b1/github/Alessandro-Salerno/hera)

Create SVG Entity-Relationship diagrams using a simple text-based formal language

## Why Hera?
ER diagrams are often used in database implementation during the initial design phases, particularly as part of a [Conceptual Design](https://mariadb.com/docs/general-resources/database-theory/database-design/database-design-phase-2-conceptual-design). 
Such diagrams are also widely used in academia, being employed by introductory and advanced DB courses alike. 

Their dominance, however, is curiously not accompanied by easy-to-use, accessible design tools. In fact, the use of rudimentary or otherwise non-application-specific software such as [draw.io](https://app.diagrams.net/) is frequently encouraged.
While sufficient, these programs and websites typically rely on visual design rather than formal, text-based descriptions, making the process both _slower_ and _harder_, especially for people with visual disabilities. 

Hera (backronym for _Hierarchical Entity-Relationship Autogenerator_) aims to provide an accessible, stable, and portable, text-based solution. The project is currently intended for academic use, though its scope and structure may allow for more advanced use cases in the future.

## Features
- Custom ER description language
- Standard SVG output
- Relationships as first-class constructs (i.e., with support for attributes)
- Support for specialization hierarchies with both total and exclusive modifiers
- Visual Studio Code (TextMate) extension and Treesitter parser
- Helpful command-line error messages
- Designed to be portable and (mostly) freestanding
- UNIX-like behavior: SVG source is printed to Standard Output, allowing for piping or other forms of straightforward redirection

## Portability
While not strictly related to database design, portability is one of Hera's core philosophies. An often overlooked aspect of so called _free software_ is the freedom to choose the host platform.
Most projects, in fact, have large dependency graphs which de-facto force users to use one of a handful of popular platforms.

Given its scope, Hera strives for easy portability, thus letting users _choose_ their operating system, compiler, and standard library without any special considerations for Hera.

## Examples
```
// University Management System
// Demonstrates specialization, inheritance, and cardinality constraints

entity Person total exclusive alias p {
    attribute "ID Number" key;
    attribute Name;
    attribute "Birth Date";
}

entity Student specifies Person {
    attribute "Enrollment Year";
    attribute Major;

    relation Enrolls (1, N);
}

entity Professor specifies Person {
    attribute Department;
    attribute "Office Number";

    relation Teaches (1, N);
}

entity Course {
    attribute Code key;
    attribute Title;
    attribute Credits;

    relation Enrolls (0, N);
    relation Teaches (1, 1);
    relation Requirements (0, N);
}

relation Enrolls {
    attribute Grade;
    attribute Semester;
}

relation Teaches {
}

relation Requirements {
}
```

A collection of example input files is available in the [`examples/`](./examples/) directory.

## Documentation
Documentation is available in the [`docs/`](./docs/) directory. Additional information is also available in [`FAQ.md`](./FAQ.md), [`CONTRIBUTING.md`](./CONTRIBUTING.md), [`CODE_OF_CONDUCT.md`](./CODE_OF_CONDUCT.md), and [`LICENSE`](./LICENSE).

## License
Hera is distributed under the BSD 2-Clause license. This only applies to the core source code and headers of the program. Third-party headers and source files included in this repository may have distinct licenses. The license is always stated at the top of each file.

See [`LICENSE`](./LICENSE) for details.

## Contributing
Hera is open to contributors, see [`CONTRIBUTING.md`](./CONTRIBUTING.md) for details.
