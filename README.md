# minECS: minimal ECS

minECS is a header-only, minimalistic Entity Component System (ECS) library designed for high performance and flexibility, with zero dependencies beyond the C++ standard library.

## Purpose

minECS provides a simple yet powerful ECS foundation suitable for games, simulations, or any application requiring efficient entity management and component storage. It emphasizes:

- Minimal API surface for ease of use and integration

- Zero dependencies (only requires a standard-compliant C++ compiler)

- High performance through sparse sets and archetype-based storage

- Compile-time component composition with flexible entity creation and iteration

## Requirements

- A C++17 (or later) compliant compiler and standard library

- No external dependencies or build steps required—just include the headers

## Usage

Simply include the minECS headers in your project and define your components and entities as needed. The library provides interfaces for creating entities, adding/removing components, and iterating over entity views efficiently.

## License

minECS is licensed under the MIT License. Use, modify, and distribute freely.