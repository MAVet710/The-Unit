# Donetsk Reference Map — Architecture-First Production Bible

## Goal

Build one of The Unit's first major urban maps from real Donetsk civilian architecture and urban morphology rather than generic "Eastern European" assets.

The target is high visual authenticity at the building level: floor counts, facade rhythm, balcony type, entrance spacing, roofline, ground-floor treatment, street furniture, transit infrastructure, courtyards and industrial edge conditions should be traceable to public references.

The initial playable district is deliberately an original composition assembled from real civilian architectural anchors and typologies. It is not intended to reproduce current military positions or a live operational map.

## Historical visual baseline

Use **pre-war / civilian Donetsk references, especially 2007–2013 photography**, as the clean architectural baseline. Mission-specific damage, abandonment, debris and weathering should be authored as independent layers so the underlying buildings remain historically legible and reusable.

This gives us:

1. abundant multi-angle public reference photography;
2. clear facade/roof/window details before severe damage obscures them;
3. a consistent art baseline;
4. reusable clean and damaged variants.

## Primary public reference families

### 1. Artema Street / central-city architecture

Wikimedia Commons provides a very large street-level archive and address-indexed building categories.

- Architecture of Donetsk: https://commons.wikimedia.org/wiki/Category:Architecture_of_Donetsk
- Buildings by address: https://commons.wikimedia.org/wiki/Category:Buildings_in_Donetsk_by_address
- Artema Street: https://commons.wikimedia.org/wiki/Category:Artema_Street,_Donetsk
- Historical Artema Street photography: https://commons.wikimedia.org/wiki/Category:Historical_photos_of_Artema_Street,_Donetsk

These references are the basis for the central boulevard scale, formal street wall, trolley/tram visual language and mixed 1920s–Soviet–post-Soviet building fabric.

### 2. Artema Street 60 — exact reference anchor

Public sources establish the important facts used by the generator:

- built in 1928;
- original constructivist design;
- originally three storeys;
- postwar reconstruction added a fourth storey;
- street facade includes a semicircular projection, tall arched/pointed stair-tower windows, white columns, parapet/balustrade treatment and strong horizontal floor layering.

References:

- Wikimedia category: https://commons.wikimedia.org/wiki/Category:60_Artema_Street,_Donetsk
- Facade photo: https://commons.wikimedia.org/wiki/File:Donetsk_artema_60.jpg
- Historical 1930 view: https://commons.wikimedia.org/wiki/File:1930._Donetsk_artema_60.jpg
- Architectural history overview: https://ru.wikipedia.org/wiki/Административное_здание_по_Артёма_60

The procedural version in `ATU_DonetskDistrictGenerator` is a **facade-proportion blockout**, not a survey model. Final production art must be rebuilt from front/oblique/side photographs and measured facade ratios before being labeled as an accurate replica.

### 3. Lenin Square / Artema 76 public building cluster

Use the multi-angle photo archive to establish formal civic-square scale, facade materials, setbacks and street edge.

- https://commons.wikimedia.org/wiki/Category:Artema_Street_76,_Donetsk

Do not duplicate the whole square 1:1 during graybox. Individual reference buildings can be modeled accurately and composed into the playable district.

### 4. Donetsk Railway Station

The station is useful as a large architectural reference anchor because public sources document both the historic core and 2012 expansion.

Reference facts:

- post-WWII station building designed in 1951;
- restored historic central volume;
- major 2012 reconstruction/expansion;
- later complex included modern concourse, transit/shopping volumes and steel/glass transition spaces.

References:

- Wikimedia station category: https://commons.wikimedia.org/wiki/Category:Donetsk_Railway_Station
- 2012 exterior: https://commons.wikimedia.org/wiki/File:Donetsk_rail_station_2012.JPG
- 2012 interior: https://commons.wikimedia.org/wiki/File:Another_angle_inside_donetsk_voksal_2012.jpg
- Architecture project, concourse: https://www.archilovers.com/projects/106713/concourse-interior.html
- Architecture project, atrium: https://www.archilovers.com/projects/155237/atrium.html

The graybox currently represents the architectural relationship — restored central/historic mass plus lower modern steel/glass transit volumes — rather than claiming exact station footprint dimensions.

### 5. Soviet residential fabric

The playable district needs the residential fabric surrounding central and industrial Donetsk, not only monuments.

Initial modular families:

- **Khrushchev-era 5-storey blocks** — simple brick/panel massing, small apartments, repeated stair sections, balconies, low floor-to-floor height, no elevator emphasis.
- **Brezhnev-era 9-storey blocks** — larger panel blocks, stronger vertical stair/elevator cores, repeated balconies/loggias and taller courtyard walls.
- **Stalin-era urban blocks** — taller floor-to-floor height, more formal frontage, raised/active ground floors and stronger street-wall behavior.

General Khrushchev-era reference context:

- https://en.wikipedia.org/wiki/Khrushchevka

For final Donetsk production assets, generic typology references are not enough: collect Donetsk-specific multi-angle photographs for every facade family before texture/final-mesh approval.

## Accuracy hierarchy

Every final building should carry a small source record. Resolve dimensions in this order:

1. documented construction/floor-count facts;
2. architectural drawings or published project information when available;
3. known object dimensions visible in photos (standard doors, windows, floor slabs, curb heights) for ratio calibration;
4. multi-angle photo matching;
5. Soviet-series/era typology priors only when the Donetsk-specific reference does not reveal a dimension.

Never convert an estimate into a claimed exact dimension.

## Per-building source record

Before a building becomes `ProductionReady`, record:

- `ReferenceId`
- real building/address or typology name
- reference era/date
- floor count
- estimated/known facade width
- estimated/known depth
- floor-to-floor height
- bay count
- entrance count
- balcony/loggia pattern
- roof/parapet type
- base/plinth treatment
- facade material
- window family
- known reconstruction history
- front/left/right/rear reference URLs
- license/attribution notes for any copied texture/photo material
- confidence: `Surveyed`, `Documented`, `PhotoMatched`, or `TypologyApproximation`

## Production workflow

### Phase A — research board

For each chosen real building:

1. collect at least one straight facade reference;
2. collect two oblique angles when available;
3. collect side/rear imagery;
4. note photo year;
5. count floors, facade bays, balconies, entrances and major vertical masses;
6. document any known reconstruction history;
7. separate permanent architecture from temporary signs/cars/awnings.

### Phase B — calibrated blockout

Use UE centimeters. Block out:

- primary mass;
- floor slabs;
- entrance recesses;
- stair/elevator cores;
- roof/parapet;
- balcony slabs/loggias;
- facade projection depth.

Do not start decorative trim until silhouette and bay spacing match references.

### Phase C — modular production mesh

Create reusable families:

- wall bay
- window bay
- balcony/loggia
- entrance
- stairwell
- corner
- parapet
- roof utility
- ground-floor retail bay
- basement/plinth

Unique landmark pieces should remain unique meshes, but repeated residential architecture should use modular kits and HISM/ISM where practical.

### Phase D — material authenticity

Donetsk material library should include:

- pale/painted stucco with patch repairs;
- pink/ochre civic plaster;
- exposed red/brown brick;
- Soviet concrete panel seams;
- glazed balcony infill;
- painted steel balcony rails;
- dark aggregate/plinth stone;
- patched asphalt;
- concrete curbs;
- trolley/tram poles and overhead-wire hardware;
- galvanized utility cabinets;
- worn commercial storefront panels.

### Phase E — damage as a layer

Keep clean building asset and damage kit separate:

- broken glazing;
- spalled plaster;
- exposed brick;
- localized facade impact;
- burned interior cards;
- rubble piles;
- boarded openings;
- abandoned vehicle/clutter layer.

This lets mission designers vary condition without destroying the reference accuracy of the base structure.

## Initial procedural district

`ATU_DonetskDistrictGenerator` currently creates an architecture-study graybox with:

- broad central boulevard and formal sidewalks;
- Artema 60 reference anchor;
- 5-storey Khrushchev-era residential courtyard;
- 9-storey Brezhnev-era blocks;
- Stalin-era street-wall blocks;
- Donetsk railway-station reference massing;
- industrial warehouse/service-yard edge;
- trolley/tram poles, shelter, kiosks and utility street furniture.

The geometry is deliberately labeled in-editor so the art team can see which sections are exact-reference candidates versus typology studies.

## Next production assets

Priority order:

1. **Artema 60 production facade** — first exact civic building study.
2. **Donetsk 5-storey residential kit** — collect specific local reference buildings and replace generic Khrushchev blockout.
3. **Donetsk 9-storey panel kit** — local balcony/end-wall/stair-core variants.
4. **central street-wall kit** — 1930s–1950s facade modules.
5. **railway station facade/atrium reference kit**.
6. **Donetsk transit/utility prop kit**.
7. **industrial Donbas warehouse/yard kit**.
8. interiors for selected buildings only; not every apartment needs full interior access.

## Acceptance rule

A building should not be called an accurate Donetsk replica merely because it "looks Soviet." Final acceptance requires traceable Donetsk-specific references and matching major proportions/silhouette. The procedural generator is the research/blockout foundation, not the final art claim.
