# 🌊 Marine Drive Cox's Bazar — 3D Resort Simulation

A real-time 3D simulation of **Marine Drive Road, Cox's Bazar, Bangladesh**, built with OpenGL (GLUT/freeglut) and C++. Walk along the beachside road, explore hotels, resorts, restaurants, and enjoy dynamic weather, day/night cycles, boats at sea, and animated traffic.

---

## 📸 Preview

> Build and run the project to explore the scene interactively.

---

## 🛠️ Build Requirements

| Tool | Details |
|------|---------|
| OS | Windows |
| IDE | [Code::Blocks](https://www.codeblocks.org/) |
| Compiler | MinGW (GCC for Windows) |
| Graphics | [freeglut](https://freeglut.sourceforge.net/) + OpenGL + GLU |

### freeglut Setup (if not already installed)

1. Download freeglut for MinGW from https://www.transmissionzero.co.uk/software/freeglut-devel/
2. Copy headers to `MinGW/include/GL/`
3. Copy `.a` lib files to `MinGW/lib/`
4. Copy `freeglut.dll` to your project's `bin/Debug/` folder

### Build Steps

1. Open `marine drive.cbp` in Code::Blocks
2. Set build target to **Debug**
3. Press **F9** (Build & Run)

Or compile manually via terminal:

```bash
g++ main3d.cpp -o "Marine Drive.exe" -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---

## 🎮 Controls

```
W A S D        : move camera (forward / back / left / right)
Q / E          : move camera up / down
Drag LMB       : look around (mouse drag)
UP / DOWN      : vehicle speed +/-
1 / 0          : animation on / off
n              : night mode
y              : day mode
u              : toggle sunset
r / k          : rain on / off
f / h          : fog on / off
c              : reset camera
+ / -          : fullscreen / windowed
x / Esc        : exit
```

---

## 🌆 Scene Features

### 🏖️ Beach & Sea
- Animated ocean waves with depth-based colour gradient
- 4 animated small boats drifting across the sea
- 7 detailed fishing trawlers (wooden hull, mast, nets, life buoy)
- 2 large passenger/cargo ships with multi-deck superstructure, funnels, radar
- Animated seagull birds flying above the beach
- Animated swimmers in the shallows

### 🛣️ Marine Drive Road
- Full 1600-unit road (±800) with lane markings
- Animated cars and vehicles driving along the road
- Lamp posts lining the entire road length

### 🏨 Buildings (left to right — 30+ structures)
- Neptune Resort & Spa, Ocean Breeze Inn, Sea Pearl Resort
- Blue Wave Bar & Lounge, Boutique Hotel, Palm Bay Hotel, Sea Mist Café
- **The Grand 5-Star Restaurant** with columns and pediment
- **Seaview Resort** (main landmark)
- Seafood restaurant, **Baywatch Resort**
- **Water Park** with slides and pool
- Beach shop, BBQ stall, Ice Cream stall
- Hotels and resorts extending to both ends of the road

### 🌴 Nature
- Dense forest with varied trees behind the buildings
- Mountain range in the background
- Bonfire with animated flames and smoke

### 🌦️ Weather & Lighting
- **Day / Sunset / Night** modes with full sky and lighting changes
- **Rain** with particle system and thunder
- **Fog** toggle
- Dynamic sky gradient and sun/moon rendering

---

## 📁 Project Structure

```
marine drive/
├── main3d.cpp          # Full source — single-file OpenGL project
├── marine drive.cbp    # Code::Blocks project file
├── bin/
│   └── Debug/
│       ├── Marine Drive.exe
│       └── freeglut.dll
└── README.md
```

---

## 👨‍💻 Author

**Rahi** — Cox's Bazar, Bangladesh  
Built as a 3D OpenGL tribute to Marine Drive, the world's longest natural sea beach road.

---

## 📄 License

This project is for educational and portfolio purposes.
