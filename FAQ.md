# Frequently Asked Questions

## Features & Current state

- **Q: Can I use Hera right now for my assignment?**

    **A:** Unfortunately, while progress continues, Hera's layout manager and
           SVG emitter are currently experimental and thus will likely produce
           overly convoluted, unreadable or outright wrong results. This is the
           main area of focus in Hera right now.

- **Q: What can I expect from future updates?**

    **A:** It is important to note that Hera is a side-project and, as such, may
           lack a regular update cycle. See the [Issues](https://github.com/Alessandro-Salerno/hera/issues)
           tab and [`CONTRIBUTING.md`](./CONTRIBUTING.md) for potential future
           changes.

## Comparisons & Philosophy

- **Q: How is Hera different from PlantUML or Mermaid.js?**

    **A:** While PlantUML and Mermaid are fantastic general-purpose diagramming
           tools, Hera is highly specialized for Entity-Relationship diagrams.
           Because it understands the _semantics_ of ER modeling (like
           total/exclusive specializations and key attributes) rather than just
           drawing shapes, it can enforce structural rules and generate
           diagrams that adhere to academic DB design standards, while also
           providing some degree of static analysis.

- **Q: Why text-based instead of a GUI?**

    **A:** Text-based definitions are inherently more accessible (especially for
           screen readers), easier to version control, and faster to draft once you
           know the syntax. Visual positioning is mostly handled automatically,
           removing the friction of manually aligning boxes and arrows. Offloading
           image manipulation and visualization to third-party software also improves
           user freedom and lifts several high-complexity responsibilities off Hera.

- **Q: Why C?**

    **A:** As stated in the [README](./README.md#Portability), one of Hera's
           core goals is to provide a fully consistent cross-platform experience
           and maximize user choice, treating platforms beyond the usual
           _big three_ (Microsoft Windows, Apple macOS, and Linux) as equally
           deserving of support. Using complex, high level, multi-dependency
           languages and toolkits, while probably beneficial in terms of code
           complexity, would significantly harm this effort.

## Usage & Output

- **Q: Hera only outputs SVG. How do I get a PNG or PDF?**
    **A:** Hera adheres to a strict principle of separation of concerns.
           The tool outputs standard SVG because it scales perfectly and is universally
           supported. To convert it to PNG or PDF, pipe the output into standard
           CLI conversion tools such as ImageMagick or Inkscape.

> [!TIP]
> ```bash
> hera model.er | inkscape --pipe --export-filename=model.png
> ```

- **Q: Can I change the colors or visual theme of the generated diagrams?**

    **A:** Hera currently doesn't provide native support for custom styles, but
           because the output is standard SVG, you can easily modify the styling using
           CSS or standard text replacement tools.

- **Q: Why does Hera print to standard output instead of saving a file?**

    **A:** Printing to `stdout` maximizes flexibility. It allows users to view
           the raw SVG directly, pipe it into other utilities (like `grep` or conversion
           tools), or redirect it to a file of their choosing, as seen below, without
           requiring Hera to manage file system permissions.

> [!TIP]
> ```bash
> hera model.er > model.svg
> ```

