Info<< "Reading field p_rgh\n" << endl;
volScalarField p_rgh
(
    IOobject
    (
        "p_rgh",
        runTime.name(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

Info<< "Reading field U\n" << endl;
volVectorField U
(
    IOobject
    (
        "U",
        runTime.name(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);

#include "createPhi.H"


#include "readGravitationalAcceleration.H"
#include "readhRef.H"
#include "gh.H"


Info<< "Reading incompressibleTwoPhaseInteractingMixture\n" << endl;
incompressibleTwoPhaseInteractingMixture mixture(U, phi);

volScalarField& alpha1(mixture.alpha1());
const volScalarField& rho(mixture.rho());

// Mass flux
surfaceScalarField rhoPhi
(
    IOobject
    (
        "rhoPhi",
        runTime.name(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    fvc::interpolate(rho)*phi
);


// Dispersed phase relative velocity model
autoPtr<relativeVelocityModel> relativeVelocity
(
    relativeVelocityModel::New(mixture, mixture, g)
);


// Construct compressible turbulence model
autoPtr<compressible::momentumTransportModel> turbulence
(
    compressible::momentumTransportModel::New(rho, U, rhoPhi, mixture)
);


volScalarField p
(
    IOobject
    (
        "p",
        runTime.name(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
    ),
    p_rgh + rho*gh
);

pressureReference pressureReference(p, p_rgh, pimple.dict());

if (p_rgh.needReference())
{
    p += dimensionedScalar
    (
        "p",
        p.dimensions(),
        pressureReference.refValue()
      - getRefCellValue(p, pressureReference.refCell())
    );
    p_rgh = p - rho*gh;
}

mesh.schemes().setFluxRequired(p_rgh.name());
mesh.schemes().setFluxRequired(alpha1.name());

// MULES Correction
tmp<surfaceScalarField> talphaPhiCorr0;

#include "createMRF.H"
#include "createFvModels.H"
#include "createFvConstraints.H"

//- Pointer to the surface momentum field
//  used for ddtCorr with MRF
autoPtr<surfaceVectorField> Uf;

if (MRF.size())
{
    Info<< "Constructing face momentum Uf" << endl;

    // Ensure the U BCs are up-to-date before constructing Uf
    U.correctBoundaryConditions();

    Uf = new surfaceVectorField
    (
        IOobject
        (
            "Uf",
            runTime.name(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        fvc::interpolate(U)
    );
}
