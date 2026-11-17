#pragma once
#include "prop_catalog.h"
#include "props_shared.h"
#include "props_office.h"
#include "props_parking.h"
#include "props_maintenance.h"
#include "props_warehouse.h"
#include "props_hotel.h"
#include "props_pool.h"
#include "props_hospital.h"
#include "props_fixtures.h"
#include "props_clutter.h"
#include "props_school.h"
#include "props_mall.h"
#include "props_playroom.h"
#include "props_station.h"
#include "props_lab.h"
#include "props_kitchen.h"

// ============================================================================
// PROP LIBRARY
// Maps the library ids of prop_catalog.h to their mesh builders.
// The base set (ids below MAP_PROP_LIBRARY_FIRST) lives in prop_models.h.
// ============================================================================

inline bool buildLibraryPropModel(DecorBuilder& b, int type) {
    switch (type) {
        case MAP_PROP_TRASH_BIN: buildTrashBinModel(b); return true;
        case MAP_PROP_BENCH: buildBenchModel(b); return true;
        case MAP_PROP_VENDING_MACHINE: buildVendingMachineModel(b); return true;
        case MAP_PROP_WATER_COOLER: buildWaterCoolerModel(b); return true;
        case MAP_PROP_FIRE_CABINET: buildFireCabinetModel(b); return true;
        case MAP_PROP_WET_FLOOR_SIGN: buildWetFloorSignModel(b); return true;
        case MAP_PROP_PIPE_RUN: buildPipeRunModel(b); return true;
        case MAP_PROP_VENT_DUCT: buildVentDuctModel(b); return true;
        case MAP_PROP_ELECTRICAL_CABINET: buildElectricalCabinetModel(b); return true;
        case MAP_PROP_STEP_LADDER: buildStepLadderModel(b); return true;
        case MAP_PROP_HAND_TRUCK: buildHandTruckModel(b); return true;
        case MAP_PROP_SHELF_UNIT: buildShelfUnitModel(b); return true;
        case MAP_PROP_HANGING_SIGN: buildHangingSignModel(b); return true;
        case MAP_PROP_FLOOR_DRAIN: buildFloorDrainModel(b); return true;
        case MAP_PROP_CEILING_FAN: buildCeilingFanModel(b); return true;
        case MAP_PROP_WALL_SIGN: buildWallSignModel(b); return true;
        case MAP_PROP_CABLE_TRAY: buildCableTrayModel(b); return true;
        case MAP_PROP_FLOOR_MAT: buildFloorMatModel(b); return true;
        case MAP_PROP_PAPER_LITTER: buildPaperLitterModel(b); return true;

        case MAP_PROP_COPIER: buildCopierModel(b); return true;
        case MAP_PROP_MEETING_TABLE: buildMeetingTableModel(b); return true;
        case MAP_PROP_SOFA: buildSofaModel(b); return true;
        case MAP_PROP_WHITEBOARD: buildWhiteboardModel(b); return true;
        case MAP_PROP_PAPER_STACKS: buildPaperStacksModel(b); return true;

        case MAP_PROP_JERSEY_BARRIER: buildJerseyBarrierModel(b); return true;
        case MAP_PROP_BOLLARD_ROW: buildBollardRowModel(b); return true;
        case MAP_PROP_TIRE_STACK: buildTireStackModel(b); return true;
        case MAP_PROP_PAY_STATION: buildPayStationModel(b); return true;
        case MAP_PROP_WHEEL_STOP: buildWheelStopModel(b); return true;
        case MAP_PROP_BARRIER_GATE: buildBarrierGateModel(b); return true;

        case MAP_PROP_BOILER_TANK: buildBoilerTankModel(b); return true;
        case MAP_PROP_VALVE_MANIFOLD: buildValveManifoldModel(b); return true;
        case MAP_PROP_GENERATOR: buildGeneratorModel(b); return true;
        case MAP_PROP_WORKBENCH: buildWorkbenchModel(b); return true;
        case MAP_PROP_TOOL_CHEST: buildToolChestModel(b); return true;
        case MAP_PROP_FLOOR_HATCH: buildFloorHatchModel(b); return true;
        case MAP_PROP_CONTROL_CONSOLE: buildControlConsoleModel(b); return true;
        case MAP_PROP_PUMP_UNIT: buildPumpUnitModel(b); return true;

        case MAP_PROP_PALLET_RACK: buildPalletRackModel(b); return true;
        case MAP_PROP_WRAPPED_PALLET: buildWrappedPalletModel(b); return true;
        case MAP_PROP_LONG_CRATE: buildLongCrateModel(b); return true;
        case MAP_PROP_PALLET_JACK: buildPalletJackModel(b); return true;
        case MAP_PROP_PALLET_PILE: buildPalletPileModel(b); return true;

        case MAP_PROP_BED: buildBedModel(b); return true;
        case MAP_PROP_NIGHTSTAND: buildNightstandModel(b); return true;
        case MAP_PROP_ARMCHAIR: buildArmchairModel(b); return true;
        case MAP_PROP_LUGGAGE_CART: buildLuggageCartModel(b); return true;
        case MAP_PROP_RECEPTION_DESK: buildReceptionDeskModel(b); return true;
        case MAP_PROP_WARDROBE: buildWardrobeModel(b); return true;

        case MAP_PROP_LOUNGE_CHAIR: buildLoungeChairModel(b); return true;
        case MAP_PROP_STARTING_BLOCK: buildStartingBlockModel(b); return true;
        case MAP_PROP_LIFEGUARD_CHAIR: buildLifeguardChairModel(b); return true;
        case MAP_PROP_TOWEL_CART: buildTowelCartModel(b); return true;
        case MAP_PROP_SHOWER_POST: buildShowerPostModel(b); return true;
        case MAP_PROP_FLOAT_RACK: buildFloatRackModel(b); return true;

        case MAP_PROP_GURNEY: buildGurneyModel(b); return true;
        case MAP_PROP_IV_STAND: buildIvStandModel(b); return true;
        case MAP_PROP_MED_CABINET: buildMedCabinetModel(b); return true;
        case MAP_PROP_PRIVACY_SCREEN: buildPrivacyScreenModel(b); return true;
        case MAP_PROP_WHEELCHAIR: buildWheelchairModel(b); return true;
        case MAP_PROP_WAITING_SEATS: buildWaitingSeatsModel(b); return true;

        case MAP_PROP_WALL_CLOCK: buildWallClockModel(b); return true;
        case MAP_PROP_PA_SPEAKER: buildPaSpeakerModel(b); return true;
        case MAP_PROP_WALL_TV: buildWallTvModel(b); return true;
        case MAP_PROP_DANGLING_LIGHT: buildDanglingLightModel(b); return true;
        case MAP_PROP_SPRINKLER_PIPE: buildSprinklerPipeModel(b); return true;
        case MAP_PROP_NOTICE_BOARD: buildNoticeBoardModel(b); return true;
        case MAP_PROP_PAYPHONE: buildPayphoneModel(b); return true;
        case MAP_PROP_RADIATOR: buildRadiatorModel(b); return true;
        case MAP_PROP_DRINKING_FOUNTAIN: buildDrinkingFountainModel(b); return true;

        case MAP_PROP_DEAD_PLANT: buildDeadPlantModel(b); return true;
        case MAP_PROP_BOOKCASE: buildBookcaseModel(b); return true;
        case MAP_PROP_JANITOR_CART: buildJanitorCartModel(b); return true;
        case MAP_PROP_STANCHIONS: buildStanchionsModel(b); return true;
        case MAP_PROP_STACKED_CHAIRS: buildStackedChairsModel(b); return true;
        case MAP_PROP_FALLEN_TILES: buildFallenTilesModel(b); return true;
        case MAP_PROP_GAS_CYLINDERS: buildGasCylindersModel(b); return true;
        case MAP_PROP_NEWSSTAND: buildNewsstandModel(b); return true;

        case MAP_PROP_SCHOOL_DESK: buildSchoolDeskModel(b); return true;
        case MAP_PROP_BLACKBOARD: buildBlackboardModel(b); return true;
        case MAP_PROP_COAT_HOOKS: buildCoatHooksModel(b); return true;
        case MAP_PROP_VAULTING_HORSE: buildVaultingHorseModel(b); return true;
        case MAP_PROP_TROPHY_CASE: buildTrophyCaseModel(b); return true;
        case MAP_PROP_AV_CART: buildAvCartModel(b); return true;
        case MAP_PROP_LECTERN: buildLecternModel(b); return true;

        case MAP_PROP_CLOTHES_RACK: buildClothesRackModel(b); return true;
        case MAP_PROP_MANNEQUIN: buildMannequinModel(b); return true;
        case MAP_PROP_SHOPPING_CART: buildShoppingCartModel(b); return true;
        case MAP_PROP_DISPLAY_COUNTER: buildDisplayCounterModel(b); return true;
        case MAP_PROP_INFO_KIOSK: buildInfoKioskModel(b); return true;
        case MAP_PROP_GONDOLA_SHELF: buildGondolaShelfModel(b); return true;
        case MAP_PROP_FITTING_BOOTH: buildFittingBoothModel(b); return true;

        case MAP_PROP_PARTY_TABLE: buildPartyTableModel(b); return true;
        case MAP_PROP_BALLOON_BUNCH: buildBalloonBunchModel(b); return true;
        case MAP_PROP_BALL_PIT: buildBallPitModel(b); return true;
        case MAP_PROP_ARCADE_CABINET: buildArcadeCabinetModel(b); return true;
        case MAP_PROP_PLAY_SLIDE: buildPlaySlideModel(b); return true;
        case MAP_PROP_SOFT_BLOCKS: buildSoftBlocksModel(b); return true;
        case MAP_PROP_GIFT_PILE: buildGiftPileModel(b); return true;
        case MAP_PROP_BUNTING: buildBuntingModel(b); return true;

        case MAP_PROP_TURNSTILE: buildTurnstileModel(b); return true;
        case MAP_PROP_TICKET_MACHINE: buildTicketMachineModel(b); return true;
        case MAP_PROP_ROUTE_MAP: buildRouteMapModel(b); return true;
        case MAP_PROP_TACTILE_STRIP: buildTactileStripModel(b); return true;
        case MAP_PROP_TICKET_BOOTH: buildTicketBoothModel(b); return true;
        case MAP_PROP_SUITCASES: buildSuitcasesModel(b); return true;

        case MAP_PROP_SERVER_RACK: buildServerRackModel(b); return true;
        case MAP_PROP_LAB_BENCH: buildLabBenchModel(b); return true;
        case MAP_PROP_FUME_HOOD: buildFumeHoodModel(b); return true;
        case MAP_PROP_SPECIMEN_TANK: buildSpecimenTankModel(b); return true;
        case MAP_PROP_TAPE_DRIVE: buildTapeDriveModel(b); return true;
        case MAP_PROP_EQUIPMENT_CART: buildEquipmentCartModel(b); return true;
        case MAP_PROP_EYEWASH_STATION: buildEyewashStationModel(b); return true;
        case MAP_PROP_CABLE_COVER: buildCableCoverModel(b); return true;

        case MAP_PROP_STOVE_RANGE: buildStoveRangeModel(b); return true;
        case MAP_PROP_FRIDGE: buildFridgeModel(b); return true;
        case MAP_PROP_PREP_COUNTER: buildPrepCounterModel(b); return true;
        case MAP_PROP_DINER_BOOTH: buildDinerBoothModel(b); return true;
        case MAP_PROP_CAFETERIA_TABLE: buildCafeteriaTableModel(b); return true;
        case MAP_PROP_TRAY_RACK: buildTrayRackModel(b); return true;
        case MAP_PROP_SERVING_LINE: buildServingLineModel(b); return true;
        case MAP_PROP_RANGE_HOOD: buildRangeHoodModel(b); return true;
        case MAP_PROP_SACK_PILE: buildSackPileModel(b); return true;
        default: return false;
    }
}
