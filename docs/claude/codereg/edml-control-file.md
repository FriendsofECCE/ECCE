---
type: map
title: "EDML control file"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: [".edml", "data/client/cap/*.edml", "src/dsm/xml/CodeFactory.C"]
---
**EDML control file** (`data/client/cap/*.edml`) — one per code, an
XML manifest naming every other piece below (`<InputGenerator>`,
`<Template>`, `<ParseSpecification>`, `<BasisSetTranslationScript>`,
theory/runtype categories, GUI dialog script names). `data/client/
config/ResourceDescriptor.xml`/`ResourceDescriptorRxn.xml` reference a
code's `.edml` to put a "New \<Code\> Calculation..." entry in the
menu — that's what makes a code actually reachable.
`CodeFactory::getFullySupportedCodes()` (`src/dsm/xml/CodeFactory.C`)
is the authoritative "is this code wired up" check (needs both
`<InputGenerator>` and `<Template>`).
