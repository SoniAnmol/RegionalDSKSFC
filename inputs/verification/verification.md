# Verification
We verify the model behaviour by replicating scenarios tested in Lamperti et al. (2018) in both regionalised SFC-DSK and comparing its results with SFC-DSK by Reissl et al. (2025).

Following scenarios are recreated.
| Scenario  | Shock configuration        | Model flags                                                            |
|-----------|--------------------------|--------------------------------------------------------------------------|
|Scenario 1 | No shocks                | "flag_shockexperiment" : 0;                                              |
|Scenario 2 | Labor Productivity (LP)  | "flag_shockexperiment" = 1; "flag_prodshocks2":1;                        |
|Scenario 3 | Energy Efficiency (EF)   | "flag_shockexperiment" = 1; "flag_energyshocks":1;                       |
|Scenario 4 | Capital Stock (CS)       | "flag_shockexperiment" = 1; "flag_capshocks":1;                          |
|Scenario 5 | Inventories (INV)        | "flag_shockexperiment" = 1; "flag_inventshocks":1;                       |
|Scenario 6 | LP & EF                  | "flag_shockexperiment" = 1; "flag_prodshocks2":1; "flag_energyshocks": 1;|
|Scenario 7 | LP & CS                  | "flag_shockexperiment" = 1; "flag_prodshocks2":1; "flag_capshocks":1;    |
|Scenario 8 | CS & EF                  | "flag_shockexperiment" = 1; "flag_capshocks":1; "flag_energyshocks":1;   |
|Scenario 9 | CS & INV                 | "flag_shockexperiment" = 1; "flag_capshocks":1; "flag_inventshocks":1;   |

