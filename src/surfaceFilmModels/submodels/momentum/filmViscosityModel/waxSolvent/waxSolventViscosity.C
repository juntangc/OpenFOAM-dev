/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2017-2022 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "waxSolventViscosity.H"
#include "momentumSurfaceFilm.H"
#include "waxSolventEvaporation.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace surfaceFilmModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(waxSolventViscosity, 0);

addToRunTimeSelectionTable
(
    viscosityModel,
    waxSolventViscosity,
    dictionary
);


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void waxSolventViscosity::correctMu()
{
    const momentumSurfaceFilm& film = filmType<momentumSurfaceFilm>();

    const uniformDimensionedScalarField Wwax
    (
        film.mesh().lookupObject<uniformDimensionedScalarField>
        (
            Foam::typedName<waxSolventEvaporation>("Wwax")
        )
    );

    const uniformDimensionedScalarField Wsolvent
    (
        film.mesh().lookupObject<uniformDimensionedScalarField>
        (
            Foam::typedName<waxSolventEvaporation>("Wsolvent")
        )
    );

    const uniformDimensionedScalarField Ysolvent0
    (
        film.mesh().lookupObject<uniformDimensionedScalarField>
        (
            Foam::typedName<waxSolventEvaporation>("Ysolvent0")
        )
    );

    const volScalarField& Ysolvent
    (
        film.mesh().lookupObject<volScalarField>
        (
            Foam::typedName<waxSolventEvaporation>("Ysolvent")
        )
    );

    const volScalarField Xsolvent
    (
        Ysolvent*Wsolvent/((1 - Ysolvent)*Wwax + Ysolvent*Wsolvent)
    );

    const dimensionedScalar Xsolvent0
    (
        Ysolvent0*Wsolvent/((1 - Ysolvent0)*Wwax + Ysolvent0*Wsolvent)
    );

    mu_ = pow(muWax_/muSolvent_, (1 - Xsolvent)/(1 - Xsolvent0))*muSolvent_;
    mu_.correctBoundaryConditions();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

waxSolventViscosity::waxSolventViscosity
(
    surfaceFilm& film,
    const dictionary& dict,
    volScalarField& mu
)
:
    viscosityModel(typeName, film, dict, mu),
    muWax_
    (
        IOobject
        (
            typedName("muWax"),
            film.mesh().time().name(),
            film.mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        film.mesh(),
        dimensionedScalar(dimDynamicViscosity, 0),
        zeroGradientFvPatchScalarField::typeName
    ),
    muWaxModel_
    (
        viscosityModel::New
        (
            film,
            coeffDict_.subDict("muWax"),
            muWax_
        )
    ),
    muSolvent_
    (
        IOobject
        (
            typedName("muSolvent"),
            film.mesh().time().name(),
            film.mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        film.mesh(),
        dimensionedScalar(dimDynamicViscosity, 0),
        zeroGradientFvPatchScalarField::typeName
    ),
    muSolventModel_
    (
        viscosityModel::New
        (
            film,
            coeffDict_.subDict("muSolvent"),
            muSolvent_
        )
    )
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

waxSolventViscosity::~waxSolventViscosity()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

void waxSolventViscosity::correct
(
    const volScalarField& p,
    const volScalarField& T
)
{
    muWaxModel_->correct(p, T);
    muSolventModel_->correct(p, T);

    correctMu();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace surfaceFilmModels
} // End namespace Foam

// ************************************************************************* //
