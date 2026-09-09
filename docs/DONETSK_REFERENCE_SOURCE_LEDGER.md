# Donetsk Building Reference Source Ledger

This ledger tracks the public civilian references used to turn the procedural Donetsk graybox into production assets. It deliberately separates **documented facts** from **photo-matched estimates** so guessed dimensions do not become fake precision.

| ID | Building / family | Donetsk-specific reference | Known facts currently safe to treat as documented | Current confidence |
|---|---|---|---|---|
| `DON_A60` | Artema Street 60 | https://commons.wikimedia.org/wiki/Category:60_Artema_Street,_Donetsk | Built 1928; original 3 floors; postwar reconstruction added fourth floor; rounded street projection, tall stair-tower windows, white columns, decorative parapet and balcony balustrades | Documented facts + PhotoMatched working dimensions |
| `DON_ARTEMA76` | Artema Street 76 / Lenin Square frontage | https://commons.wikimedia.org/wiki/Category:Artema_Street_76,_Donetsk | 27-image multi-angle public category suitable for facade/setback study | PhotoMatched pending measurement |
| `DON_ARTEMA119` | Artema Street 119 cluster | https://commons.wikimedia.org/wiki/Category:Artema_Street_119,_Donetsk | Multiple street/oblique views including 2012 imagery | PhotoMatched pending measurement |
| `DON_ILY20` | Ilicha Avenue 20 / 20a residential high-rise | https://commons.wikimedia.org/wiki/Category:Ilicha_Avenue,_Donetsk_(part_in_Kalininskyi_Raion) | Donetsk-specific high-rise residential reference; public images show local facade/end-wall/balcony treatment | PhotoMatched pending manual floor/bay count |
| `DON_ILY24` | Ilicha Avenue 24 / 26 residential blocks | https://commons.wikimedia.org/wiki/Category:Ilicha_Avenue,_Donetsk_(part_in_Kalininskyi_Raion) | Donetsk-specific residential block pair suitable for local panel/brick kit study | PhotoMatched pending manual floor/bay count |
| `DON_ROZY_DORM` | Donetsk National University dormitory family, Rozy Luxemburg Street | https://commons.wikimedia.org/wiki/Category:Rozy_Luxemburg_Street,_Donetsk | 18-photo dormitory subcategory plus street photography for urban context | PhotoMatched pending extraction |
| `DON_STATION` | Donetsk Railway Station | https://commons.wikimedia.org/wiki/Category:Donetsk_Railway_Station | Postwar core designed 1951; major 2012 reconstruction/expansion; public exterior/interior archives | Documented history + PhotoMatched massing |
| `DON_ARTEMA_TRANSIT` | Artema Street transit/street furniture | https://commons.wikimedia.org/wiki/Category:Historical_photos_of_Artema_Street,_Donetsk | Historic tram/trolley imagery and broad main-street context | PhotoMatched urban grammar |

## Artema 60 full multi-angle reference set

Primary category:

- https://commons.wikimedia.org/wiki/Category:60_Artema_Street,_Donetsk

At the time the dedicated production-reference workflow was added, the category listed **14 media files**, including the 1930 historical configuration and multiple modern/current facade views.

Key references:

- current facade: https://commons.wikimedia.org/wiki/File:Donetsk_artema_60.jpg
- current oblique facade: https://commons.wikimedia.org/wiki/File:Donetsk_artema_60_2.jpg
- 1930 three-storey historical configuration: https://commons.wikimedia.org/wiki/File:1930._Donetsk_artema_60.jpg
- building-history article: https://infodon.org.ua/time/201
- architecture catalog: https://photobuildings.com/object/245491/
- Docomomo Ukraine modernism overview: https://docomomojournal.com/index.php/journal/article/download/543/445

The documented/history data and the current working dimensions are recorded separately in:

- `Tools/Reference/artema60_calibration.json`

The current working dimensions remain `PhotoMatchedWorkingEstimate`, not `Surveyed`.

Download the complete current Wikimedia category locally with:

```powershell
.\Tools\Reference\get_artema60_reference_set.ps1
```

The script stores the files under the git-ignored `ExternalAssets/DonetskReferences/Artema60_FullCategory` directory and creates `artema60_reference_ledger.json` with the Wikimedia source URL, image dimensions, author and license metadata for every downloaded file.

## Additional reference indexes

- Architecture of Donetsk: https://commons.wikimedia.org/wiki/Category:Architecture_of_Donetsk
- Buildings in Donetsk by address: https://commons.wikimedia.org/wiki/Category:Buildings_in_Donetsk_by_address
- Buildings in Voroshylovskyi Raion: https://commons.wikimedia.org/wiki/Category:Buildings_in_Voroshylovskyi_Raion_of_Donetsk
- Artema Street: https://commons.wikimedia.org/wiki/Category:Artema_Street,_Donetsk
- Ilicha Avenue, Kalininskyi Raion: https://commons.wikimedia.org/wiki/Category:Ilicha_Avenue,_Donetsk_(part_in_Kalininskyi_Raion)
- Donetsk Railway Station: https://commons.wikimedia.org/wiki/Category:Donetsk_Railway_Station

## Accuracy grades

### `Documented`
Use only when a source explicitly gives the fact: construction year, floor count, architect, documented reconstruction, published project area, etc.

### `PhotoMatched`
A dimension or detail was derived by comparing multiple views and known-size architectural objects. Keep the underlying evidence in the local reference pack.

### `TypologyApproximation`
Used only when a Donetsk-specific photograph does not expose enough information. This is acceptable for graybox/repeated background structures but must not be presented as an exact replica.

### `Surveyed`
Reserved for future measured drawings, licensed scan/photogrammetry, reliable building plans or comparable dimensional control. None of the current reference assets should be labeled Surveyed yet.

## Required measurement sheet before final mesh approval

For every exact-reference building, record:

- floor count;
- estimated/known floor-to-floor height;
- facade bay count;
- window width/height ratio;
- bay center spacing;
- entrance count and spacing;
- balcony/loggia count and projection depth;
- facade projection/recess depths;
- parapet/roof shape;
- side-wall depth;
- plinth/base height;
- major material transitions;
- front/side/rear source links;
- uncertainty notes.

## Local reference commands

General Donetsk pack:

```powershell
.\Tools\Reference\get_donetsk_reference_pack.ps1
```

Artema 60 complete building category:

```powershell
.\Tools\Reference\get_artema60_reference_set.ps1
```

Downloaded media stays under `ExternalAssets/`, which is git-ignored. Review each source license before redistributing images or using them directly as texture content.
