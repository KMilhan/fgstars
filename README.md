# A Desktop Planetarium for KDE

KStars is free, open source, cross-platform Astronomy Software.

It provides an accurate graphical simulation of the night sky, from any location on Earth, at any date and time. The display includes up to 100 million stars, 13,000 deep-sky objects,all 8 planets, the Sun and Moon, and thousands of comets, asteroids, supernovae, and satellites.

For students and teachers, it supports adjustable simulation speeds in order to view phenomena that happen over long timescales, the KStars Astrocalculator to predict conjunctions, and many common astronomical calculations. For the amateur astronomer, it provides an observation planner, a sky calendar tool, and an FOV editor to calculate field of view of equipment and display them. Find out interesting objects in the "What's up Tonight" tool, plot altitude vs. time graphs for any object, print high-quality sky charts, and gain access to lots of information and resources to help you explore the universe!

Included with KStars is Ekos astrophotography suite, a complete astrophotography solution that can control all INDI devices including numerous telescopes, CCDs, DSLRs, focusers, filters, and a lot more. Ekos supports highly accurate tracking using online and offline astrometry solver, autofocus and autoguiding capabilities, and capture of single or multiple images using the powerful built in sequence manager.

## Downloads

KStars is available for [Windows](https://apps.microsoft.com/detail/9pprz2qhlxtg), [MacOS](https://apps.apple.com/app/kstars-desktop-planetarium/id6759097263), and [Linux](https://kstars.kde.org/download/linux). You can download the latest version from [KStars official website](https://kstars.kde.org). Under Linux, it is recommened to use the official [KStars Flatpak](https://flathub.org/en/apps/org.kde.kstars).

## Important URLs and files.

* The [KStars homepage](https://kstars.kde.org)
* KStars [Git Repository](https://invent.kde.org/education/kstars)
* KStars [Web Chat](https://webchat.kde.org/#/room/#kstars:kde.org)
* Forum [where KStars is often discussed](https://indilib.org/forum.html)

## KStars documentation

KStars [online documentation](https://kstars.kde.org/) is avaialble in several languages. It is writtetn using Restructured Text and maintain ined in the KStars [Documentation Repository](https://invent.kde.org/documentation/kstars-docs-kde-org).

We welcome any improvements to the online documentation!

In addition, there are the following README files:

README:             This file; general information
README.planetmath:  Explanation of algorithms used to compute planet positions
README.customize:   Advanced customization options
README.images:      Copyright information for images used in KStars.
README.i18n:        Instructions for translators

## Development

Code can be cloned, viewed and merge requests can be made via the [KStars repository](https://invent.kde.org/education/kstars).

### Integrated Development Environment IDE

If you plan to develop KStars, it is highly recommended to utilize an Integrated Development Environment (IDE) You can use any IDE of your choice, but [Visual Studio Code](https://code.visualstudio.com/),  [Qt Creator](https://www.qt.io/product/development-tools) or [KDevelop](https://www.kdevelop.org) are recommended as they are more suited for Qt/KDE development.

To open KStars in QtCreator, select the CMakeLists.txt file in the KStars source folder and then configure the build location and type.

### Debugging with Visual Studio Code

If you are using Visual Studio Code for development, KStars includes pre-configured debug configurations for both GDB and LLDB debuggers.

#### Installing LLDB (Recommended)

For the best debugging experience with custom formatters for KStars astronomy types (like `dms`, `SkyPoint`, `SkyObject`), we recommend using LLDB:

**Debian/Ubuntu:**
```bash
sudo apt-get install lldb
```

**Arch Linux:**
```bash
sudo pacman -S lldb
```

**Other distributions:** Install the `lldb` package using your distribution's package manager.

#### Visual Studio Code Setup

1. Install the recommended VS Code extensions (the editor will prompt you when opening the project)
2. The CodeLLDB extension (`vadimcn.vscode-lldb`) is recommended for LLDB debugging
3. Select the "Debug KStars" configuration from the Run and Debug panel
4. Custom formatters in `.lldb/formatters/` will automatically display KStars types in human-readable format

For more information about the custom formatters, see `.lldb/formatters/README.md`.

### Building

1. Prerequisite Packages

To build and develop KStars, several packages may be required from your distribution. Here's a list.

* Required dependencies
    * GNU Make, GCC -- Essential tools for building
    * cmake -- buildsystem used by KDE
    * Qt Library > 6.6.0
    * Several KDE Frameworks: KConfig, KGuiAddons, KWidgetsAddons, KNewStuff, KI18n, KInit, KIO, KXmlGui, KPlotting, KIconThemes
    * eigen -- linear algebra library
    * zlib -- compression library
    * StellarSolver -- see [https://github.com/rlancaste/stellarsolver](https://github.com/rlancaste/stellarsolver)

* Optional dependencies
    * libcfitsio -- FITS library
    * libindi -- Instrument Neutral Distributed Interface, for controlling equipment.
    * xplanet
    * astrometry.net
    * libraw
    * wcslib
    * libgsl
    * libxisf
    * qtkeychain
    * sentry


2. Installing Prerequisites

Debian/Ubuntu

If your distribution does not provide `libstellarsolver-dev`, you will need to build and install it from https://github.com/rlancaste/stellarsolver. Remove `libstellarsolver-dev` from the `apt-get` command below if you plan to build it from source.
```
sudo apt-get -y install build-essential cmake ninja-build git libstellarsolver-dev libxisf-dev libeigen3-dev libcfitsio-dev zlib1g-dev libindi-dev extra-cmake-modules libkf6plotting-dev libqt6svg6-dev libkf6xmlgui-dev kio-dev kinit-dev libkf6newstuff-dev libkf6notifications-dev qt6-declarative-dev libkf6crash-dev gettext libnova-dev libgsl-dev libraw-dev libkf6notifyconfig-dev wcslib-dev libqt6websockets6-dev xplanet xplanet-images qtkeychain-qt6-dev libsecret-1-dev breeze-icon-theme libqt6datavisualization6-dev libcurl4-gnutls-dev
```

Fedora

Use Qt6 packages for Fedora builds. For optimal compatibility, use a recent Fedora release or another current distribution with KDE Frameworks 6 packages.

Arch Linux packages for Qt6:
```
sudo pacman -S base-devel cmake ninja git eigen cfitsio zlib extra-cmake-modules kplotting qt6-svg kxmlgui kio knewstuff kdoctools knotifications qt6-declarative kcrash gettext libxisf libnova gsl libraw knotifyconfig wcslib qt6-websockets xplanet qtkeychain-qt6 libsecret breeze-icons qt6-quick3d curl stellarsolver
```

3. Compiling

Open a console and run in the following commands:
```
git clone https://invent.kde.org/education/kstars.git
cd kstars
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
```

To run KStars without installing, simply run the executable from the build directory:
```
./bin/kstars
```

### Code Style

KStars uses [Artistic Style](http://astyle.sourceforge.net) to format all C++ source files. The repository includes a matching `.astylerc` and a helper script `tools/run_astyle.sh` which you can run from the repo root.
Please ensure you run this script to format your code before submitting a Pull Request.

```sh
# Format all modified sources
bash tools/run_astyle.sh
```

### Making Updates to the Manual

KStars online documentation is hosted 
The source for the handbook is in kstars/doc.
You can edit those files, include them in commits and MRs like you would c++ files (see below).
You could figure out the markup by example, or learn from [online doc for docbook](https://opensource.com/article/17/9/docbook).
In general, it is best to first copy the entire kstars/doc directory to a temporary directory, and edit and generate the handbook there,
because if you ran meinproc in the main source directory, you would generate many .html files there,
and you don't want to commit the generated files to your git repository.

```
cp -pr kstars/doc ~/DOCBOOK
cd ~/DOCBOOK
meinproc6 index.docbook
```

The above should generate html files. Then, in a browser, you can simply open DOCBOOK/index.html and navigate your way to the part you want, e.g. just type something similar to this in the url bar of chrome: file:///home/YOUR_USER_NAME/DOCBOOK/doc/tool-ekos.html
Make changes to some of the .docbook files in ~/DOCBOOK/*.docbook. Regenerate the html files, and view your changes in the browser, as before. Iterate.

To check syntax, you might want to run:

```
checkXML5 index.docbook
```

Once you're happy, copy your modified files back to kstars/doc, and treat the edited/new files as usual with git,
including your modified files in a new commit and eventually a new merge request.

### Pull Request Guidelines

When creating a pull request, please keep the description focused and brief, clearly mentioning the benefits of the changes.

We welcome AI-generated code contributions. However, we highly recommend using AI tools to "review" your code before submission. If you do so, please explicitly state which AI model was used for the code review in your PR description.

## Writing Tests

The test suite ensures code quality, prevents regressions, and helps document expected behavior.

For comprehensive instructions on how to write, build, and run the unitary and UI tests, please refer to the [Test Suite Documentation](Tests/README.md).

## Credits
### The KStars Team
KStars is developed and maintained by the KStars Team. We are deeply grateful to all contributors who have dedicated their time and effort to make KStars a powerful and feature-rich planetarium and astrophotography suite.

### Data Sources:

 Most of the catalog data came from the Astronomical Data Center, run by
 NASA.  The website is:
 http://adc.gsfc.nasa.gov/

 NGC/IC data is compiled by Christian Dersch from OpenNGC database.
 https://github.com/mattiaverga/OpenNGC (CC-BY-SA-4.0 license)

 Supernovae data is from the Open Supernova Catalog project at https://sne.space
 Please refer to the published paper here: http://adsabs.harvard.edu/abs/2016arXiv160501054G

 KStars links to the excellent image collections and HTML pages put together
 by the Students for the Exploration and Development of Space, at:
 http://www.seds.org

 KStars links to the online Digitized Sky Survey images, which you can
 query at:
 http://archive.stsci.edu/cgi-bin/dss_form

 KStars links to images from the HST Heritage project, and from HST
 press releases:
 http://heritage.stsci.edu
 http://oposite.stsci.edu/pubinfo/pr.html

 KStars links to images from the Advanced Observer Program at
 Kitt Peak National Observatory.  If you are interested in astrophotography,
 you might consider checking out their program:
 http://www.noao.edu/outreach/aop/

 Credits for each image used in the program are listed in README.images


# Original Acknowledgement from the Project Founder

 KStars is a labor of love.  It started as a personal hobby of mine, but
 very soon after I first posted the code on Sourceforge, it started to
 attract other developers.  I am just completely impressed and gratified
 by my co-developers.  I couldn't ask for a more talented, friendly crew.
 It goes without saying that KStars would be nowhere near what it is today
 without their efforts.  Together, we've made something we can all be
 proud of.

 We used (primarily) two books as a guide in writing the algorithms used
 in KStars:
 + "Practical Astronomy With Your Calculator" by Peter Duffett-Smith
 + "Astronomical Algorithms" by Jean Meeus

 Thanks to the developers of Qt and KDE whose peerless API made KStars
 possible.  Thanks also to the tireless efforts of the KDE translation
 teams, who bring KStars to a global audience.

 Thanks to everyone at the KDevelop message boards and on irc.kde.org,
 for answering my frequent questions.

 Thanks also to the many users who have submitted bug reports or other
 feedback.


You're still reading this? :)
Well, that's about it.  I hope you enjoy KStars!

Jason Harris
kstars@30doradus.org

KStars Development Mailing list
kstars-devel@kde.org

Send us ideas and feedback!
