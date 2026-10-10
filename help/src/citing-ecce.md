# How to cite ECCE

If you use ECCE in work you publish, please cite it. A citation shows that
the software is used, which helps keep it maintained.

## The software

Cite the DOI for all versions of ECCE:

> C. André Ohlin, Matthew Asplund and FriendsofECCE, *ECCE: Extensible
> Computational Chemistry Environment*, Zenodo,
> [https://doi.org/10.5281/zenodo.23281771](https://doi.org/10.5281/zenodo.23281771).

To cite the exact version you used, open that DOI: the Zenodo page lists
every release with its own DOI. **Help > About ECCE** shows which version
you have.

BibTeX:

```
@software{ECCE,
  author    = {Ohlin, C. Andr{\'e} and Asplund, Matthew and {FriendsofECCE}},
  title     = {{ECCE}: Extensible Computational Chemistry Environment},
  publisher = {Zenodo},
  doi       = {10.5281/zenodo.23281771},
  url       = {https://doi.org/10.5281/zenodo.23281771}
}
```

The repository's "Cite this repository" button on GitHub gives the same
reference in other formats.

## The original ECCE

ECCE was developed at Pacific Northwest National Laboratory. Its design is
described in:

- G. Black, K. Schuchardt, D. Gracio and B. Palmer, "The Extensible
  Computational Chemistry Environment: A Problem Solving Environment for
  High Performance Theoretical Chemistry", in *Computational Science –
  ICCS 2003*, Lecture Notes in Computer Science, pp. 122–131 (2003),
  [doi:10.1007/3-540-44864-0_13](https://doi.org/10.1007/3-540-44864-0_13).
- K. Schuchardt, B. Didier and G. Black, "Ecce – a problem-solving
  environment's evolution toward Grid services and a Web architecture",
  *Concurrency and Computation: Practice and Experience* **14**, 1221–1239
  (2002), [doi:10.1002/cpe.673](https://doi.org/10.1002/cpe.673).

## The codes

ECCE sets up and runs calculations; the results come from the program that
ran them (NWChem, Gaussian, ORCA, MOPAC, Quantum ESPRESSO or ECCE-QM).
Please also cite that program as its authors ask. ECCE-QM uses the libcint
and libxc libraries:

- Q. Sun, "Libcint: An efficient general integral library for Gaussian
  basis functions", *J. Comput. Chem.* **36**, 1664–1671 (2015),
  [doi:10.1002/jcc.23981](https://doi.org/10.1002/jcc.23981).
- S. Lehtola, C. Steigemann, M. J. T. Oliveira and M. A. L. Marques,
  "Recent developments in libxc – A comprehensive library of functionals
  for density functional theory", *SoftwareX* **7**, 1–5 (2018),
  [doi:10.1016/j.softx.2017.11.002](https://doi.org/10.1016/j.softx.2017.11.002).
