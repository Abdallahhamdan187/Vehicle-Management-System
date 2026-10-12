# Vehicle Access Management System (HTU & KHBP)

A database-programming project written in **C** that models a vehicle gate-access system for **HTU** and the **KHBP** tenant companies. It implements a small in-memory relational database engine from scratch (tables, hash indexes, foreign-key checks, CSV persistence, audit logging and table-level concurrency control) and uses it to decide whether a vehicle may enter through a gate.

> Course project – Database Programming, Al-Hussein Technical University (HTU)

---

## Scenario

HTU and KHBP have controlled-access gates. Each gate belongs to one or more **zones** (VIP, Employee, Student, Visitor). Users belong to one **organization** and hold one **role** (student, faculty, staff, tenant employee, visitor). Users register vehicles, and each vehicle is assigned to a single zone based on its parking permission.

Administrators define **permissions** that allow or deny access at a specific gate for a role or a vehicle. When a camera reads a license plate at a gate, the system identifies the vehicle, its owner, the owner's role and the vehicle's zone, checks the permissions, and then grants or rejects entry. **Every attempt is written to the access log**, so security officers have a trustworthy history of vehicle activity.

---

## Features

- **9 tables** with full CRUD (insert / update / delete / search by ID / list all)
- **Foreign-key enforcement** on insert, update and delete (e.g. you cannot delete a role that a permission still references)
- **Gate-entry validation** (`register_gate_entry`) that writes an `ALLOWED` / `DENIED` record with a reason
- **Hash indexes** (open addressing, size 1013) for primary-key lookups
- **CSV persistence** – data is loaded at start-up and saved on exit
- **Change audit log** – every insert/update/delete is appended to `db_log.csv` with old value, new value, action, timestamp and the username entered at session start
- **Concurrency-control simulation** – multi-threaded demo (pthreads) using shared (S) and exclusive (X) table locks
- Menu-driven console interface

---

## Database design

| Table | Key fields | Notes |
|---|---|---|
| `Organization` | `OrgID`, `OrgName`, `OrgType` | HTU or a KHBP tenant company |
| `Role` | `role_id`, `name`, `description` | One role per user |
| `User` | `user_id`, names, email, phone, `status`, `OrgID`, `role_id` | FK → Organization, Role |
| `Zone` | `ZoneID`, `ZoneName`, `description` | VIP, Employee, Student, Visitor |
| `Gate` | `gate_id`, `name`, `enabled`, `location` | |
| `GateZone` | (`ZoneID`, `gate_id`) | Many-to-many link between gates and zones |
| `Vehicle` | `vehicle_id`, `license_plate`, make, model, color, `status`, `ZoneID`, `user_id` | FK → Zone, User |
| `Permission` | `permission_id`, `target_type`, `target_id`, `action`, `created_by`, `gate_id`, `role_id` | allow/deny rule for a gate; FK → Gate, Role |
| `AccessLog` | `log_id`, `timestamp`, `outcome`, `reason`, `gate_id`, `vehicle_id`, `user_id` | FK → Gate, Vehicle, User |

Each table is stored as a singly linked list of rows, with a table-controller struct holding the head pointer, row count and next auto-increment ID.

---

## How gate validation works

`register_gate_entry(vehicle_id, gate_id)` runs these checks in order and logs the result:

1. Vehicle exists → otherwise **DENIED – Vehicle NOT FOUND**
2. Vehicle's owner exists → otherwise **DENIED – User NOT FOUND**
3. Gate exists and is enabled → otherwise **DENIED – Gate invalid/disabled**
4. Gate is assigned to the vehicle's zone (via `GateZone`) → otherwise **DENIED – Gate not assigned to vehicle zone**
5. A matching **deny** rule (vehicle-based or role-based) for that gate → **DENIED – Explicit DENY rule**
6. A matching **allow** rule (vehicle-based or role-based) for that gate → **ALLOWED – Permission matched**
7. Otherwise → **DENIED – No permission**

Deny rules always take priority over allow rules.

---

## Concurrency control demo

Menu option **11** simulates several users working at the same time. For each simulated user you choose a table and an operation (select / insert / update / delete); each user then runs in its own thread.

- **Select** takes a **shared (S) lock** – many readers allowed at once
- **Insert / update / delete** take an **exclusive (X) lock** – blocks everyone else on that table
- A blocked user prints `Wait` and retries every second until the lock is free

---

## Getting started

### Requirements

- A C compiler (GCC / MinGW-w64 / Clang)
- POSIX threads (`pthread`)

### Build

```bash
gcc Final_DB_Code.c -o vehicle_access -pthread
```

### Run

1. Create a folder named `csvFiles` next to the executable (the program saves its tables there).
2. Start the program:

```bash
./vehicle_access        # Linux / macOS
vehicle_access.exe      # Windows
```

3. Enter your name when asked (it is recorded in the audit log), then use the menu.

> **Note on file paths:** the source uses Windows-style paths (`csvFiles\\users.csv`, etc.), so it is written for Windows. On Linux or macOS, change the backslashes to forward slashes (`csvFiles/users.csv`) in `Final_DB_Code.c` before building.

### Menu overview

```
=== TABLES ===
1) Organizations   2) Roles      3) Users
4) Zones           5) Gates      6) GateZone
7) Vehicles        8) Permissions
9) Access Logs
10) List All
11) Concurrency Control Test
0) Exit            (saves all tables to CSV)
```

Typical workflow: add an organization → role → user → zone → gate → link gate to zone → add a vehicle → add a permission → use **Access Logs → Register Gate Entry** to test a vehicle at a gate.

---

## Project structure

```
.
├── Final_DB_Code.c                 # entire application (tables, indexes, CRUD, validation, logging, concurrency)
├── csvFiles/                       # created by you; data and audit log are stored here
│   ├── organizations.csv
│   ├── roles.csv
│   ├── users.csv
│   ├── zones.csv
│   ├── gates.csv
│   ├── gate_zone.csv
│   ├── vehicles.csv
│   ├── permissions.csv
│   ├── access_log.csv
│   └── db_log.csv                  # change audit log
├── docs/
│   └── Database_programming.docx   # project scenario and extended scenario
└── README.md
```

---

## Authors

- **Abdallah Hamdan** – Computer Science, Al-Hussein Technical University (HTU)

## License

Add a license of your choice (for example MIT) if you want others to reuse the code.
