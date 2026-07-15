// This file was generated from docs/smbdis.asm by the tool in codegen, and has
// since been corrected by hand where the translation was not faithful to the
// console. Regenerating it discards those corrections: see the note in
// codegen/CMakeLists.txt before you do.
//
#ifndef SMBDATAPOINTERS_HPP
#define SMBDATAPOINTERS_HPP

#include <cstdint>

// Data Addresses (16-bit pointers) for Constants
//
struct SMBDataPointers
{
    uint16_t WSelectBufferTemplate_ptr;
    uint16_t GameText_ptr; // alias
    uint16_t TopStatusBarLine_ptr;
    uint16_t WorldLivesDisplay_ptr;
    uint16_t TwoPlayerTimeUp_ptr;
    uint16_t OnePlayerTimeUp_ptr;
    uint16_t TwoPlayerGameOver_ptr;
    uint16_t OnePlayerGameOver_ptr;
    uint16_t WarpZoneWelcome_ptr;
    uint16_t WarpZoneNumbers_ptr;
    uint16_t Palette0_MTiles_ptr;
    uint16_t Palette1_MTiles_ptr;
    uint16_t Palette2_MTiles_ptr;
    uint16_t Palette3_MTiles_ptr;
    uint16_t WaterPaletteData_ptr;
    uint16_t GroundPaletteData_ptr;
    uint16_t UndergroundPaletteData_ptr;
    uint16_t CastlePaletteData_ptr;
    uint16_t DaySnowPaletteData_ptr;
    uint16_t NightSnowPaletteData_ptr;
    uint16_t MushroomPaletteData_ptr;
    uint16_t BowserPaletteData_ptr;
    uint16_t MarioThanksMessage_ptr;
    uint16_t LuigiThanksMessage_ptr;
    uint16_t MushroomRetainerSaved_ptr;
    uint16_t PrincessSaved1_ptr;
    uint16_t PrincessSaved2_ptr;
    uint16_t WorldSelectMessage1_ptr;
    uint16_t WorldSelectMessage2_ptr;
    uint16_t PlayerBGPriorityData_ptr;
    uint16_t GameTimerData_ptr;
    uint16_t VerticalPipeData_ptr;
    uint16_t SolidBlockMetatiles_ptr;
    uint16_t BrickMetatiles_ptr;
    uint16_t WorldAddrOffsets_ptr;
    uint16_t AreaAddrOffsets_ptr; // alias
    uint16_t World1Areas_ptr;
    uint16_t World2Areas_ptr;
    uint16_t World3Areas_ptr;
    uint16_t World4Areas_ptr;
    uint16_t World5Areas_ptr;
    uint16_t World6Areas_ptr;
    uint16_t World7Areas_ptr;
    uint16_t World8Areas_ptr;
    uint16_t E_CastleArea1_ptr;
    uint16_t E_CastleArea2_ptr;
    uint16_t E_CastleArea3_ptr;
    uint16_t E_CastleArea4_ptr;
    uint16_t E_CastleArea5_ptr;
    uint16_t E_CastleArea6_ptr;
    uint16_t E_GroundArea1_ptr;
    uint16_t E_GroundArea2_ptr;
    uint16_t E_GroundArea3_ptr;
    uint16_t E_GroundArea4_ptr;
    uint16_t E_GroundArea5_ptr;
    uint16_t E_GroundArea6_ptr;
    uint16_t E_GroundArea7_ptr;
    uint16_t E_GroundArea8_ptr;
    uint16_t E_GroundArea9_ptr;
    uint16_t E_GroundArea10_ptr;
    uint16_t E_GroundArea11_ptr;
    uint16_t E_GroundArea12_ptr;
    uint16_t E_GroundArea13_ptr;
    uint16_t E_GroundArea14_ptr;
    uint16_t E_GroundArea15_ptr;
    uint16_t E_GroundArea16_ptr;
    uint16_t E_GroundArea17_ptr;
    uint16_t E_GroundArea18_ptr;
    uint16_t E_GroundArea19_ptr;
    uint16_t E_GroundArea20_ptr;
    uint16_t E_GroundArea21_ptr;
    uint16_t E_GroundArea22_ptr;
    uint16_t E_UndergroundArea1_ptr;
    uint16_t E_UndergroundArea2_ptr;
    uint16_t E_UndergroundArea3_ptr;
    uint16_t E_WaterArea1_ptr;
    uint16_t E_WaterArea2_ptr;
    uint16_t E_WaterArea3_ptr;
    uint16_t L_CastleArea1_ptr;
    uint16_t L_CastleArea2_ptr;
    uint16_t L_CastleArea3_ptr;
    uint16_t L_CastleArea4_ptr;
    uint16_t L_CastleArea5_ptr;
    uint16_t L_CastleArea6_ptr;
    uint16_t L_GroundArea1_ptr;
    uint16_t L_GroundArea2_ptr;
    uint16_t L_GroundArea3_ptr;
    uint16_t L_GroundArea4_ptr;
    uint16_t L_GroundArea5_ptr;
    uint16_t L_GroundArea6_ptr;
    uint16_t L_GroundArea7_ptr;
    uint16_t L_GroundArea8_ptr;
    uint16_t L_GroundArea9_ptr;
    uint16_t L_GroundArea10_ptr;
    uint16_t L_GroundArea11_ptr;
    uint16_t L_GroundArea12_ptr;
    uint16_t L_GroundArea13_ptr;
    uint16_t L_GroundArea14_ptr;
    uint16_t L_GroundArea15_ptr;
    uint16_t L_GroundArea16_ptr;
    uint16_t L_GroundArea17_ptr;
    uint16_t L_GroundArea18_ptr;
    uint16_t L_GroundArea19_ptr;
    uint16_t L_GroundArea20_ptr;
    uint16_t L_GroundArea21_ptr;
    uint16_t L_GroundArea22_ptr;
    uint16_t L_UndergroundArea1_ptr;
    uint16_t L_UndergroundArea2_ptr;
    uint16_t L_UndergroundArea3_ptr;
    uint16_t L_WaterArea1_ptr;
    uint16_t L_WaterArea2_ptr;
    uint16_t L_WaterArea3_ptr;
    uint16_t FireballXSpdData_ptr;
    uint16_t Bubble_MForceData_ptr;
    uint16_t BubbleTimerData_ptr;
    uint16_t BrickQBlockMetatiles_ptr;
    uint16_t FlameYPosData_ptr;
    uint16_t Bitmasks_ptr;
    uint16_t FirebarMirrorData_ptr;
    uint16_t FirebarTblOffsets_ptr;
    uint16_t FirebarYPos_ptr;
    uint16_t LakituDiffAdj_ptr;
    uint16_t BridgeCollapseData_ptr;
    uint16_t ResidualXSpdData_ptr;
    uint16_t ClimbXPosAdder_ptr;
    uint16_t ClimbPLocAdder_ptr;
    uint16_t PlayerGfxTblOffsets_ptr;
    uint16_t PlayerGraphicsTable_ptr;
    uint16_t XOffscreenBitsData_ptr;
    uint16_t DefaultXOnscreenOfs_ptr;
    uint16_t MusicHeaderData_ptr;
    uint16_t TimeRunningOutHdr_ptr;
    uint16_t Star_CloudHdr_ptr;
    uint16_t EndOfLevelMusHdr_ptr;
    uint16_t ResidualHeaderData_ptr;
    uint16_t UndergroundMusHdr_ptr;
    uint16_t SilenceHdr_ptr;
    uint16_t CastleMusHdr_ptr;
    uint16_t VictoryMusHdr_ptr;
    uint16_t GameOverMusHdr_ptr;
    uint16_t WaterMusHdr_ptr;
    uint16_t WinCastleMusHdr_ptr;
    uint16_t GroundLevelPart1Hdr_ptr;
    uint16_t GroundLevelPart2AHdr_ptr;
    uint16_t GroundLevelPart2BHdr_ptr;
    uint16_t GroundLevelPart2CHdr_ptr;
    uint16_t GroundLevelPart3AHdr_ptr;
    uint16_t GroundLevelPart3BHdr_ptr;
    uint16_t GroundLevelLeadInHdr_ptr;
    uint16_t GroundLevelPart4AHdr_ptr;
    uint16_t GroundLevelPart4BHdr_ptr;
    uint16_t GroundLevelPart4CHdr_ptr;
    uint16_t DeathMusHdr_ptr;
    uint16_t Star_CloudMData_ptr;
    uint16_t GroundM_P1Data_ptr;
    uint16_t SilenceData_ptr;
    uint16_t GroundM_P2AData_ptr;
    uint16_t GroundM_P2BData_ptr;
    uint16_t GroundM_P2CData_ptr;
    uint16_t GroundM_P3AData_ptr;
    uint16_t GroundM_P3BData_ptr;
    uint16_t GroundMLdInData_ptr;
    uint16_t GroundM_P4AData_ptr;
    uint16_t GroundM_P4BData_ptr;
    uint16_t DeathMusData_ptr;
    uint16_t GroundM_P4CData_ptr;
    uint16_t CastleMusData_ptr;
    uint16_t GameOverMusData_ptr;
    uint16_t TimeRunOutMusData_ptr;
    uint16_t WinLevelMusData_ptr;
    uint16_t UndergroundMusData_ptr;
    uint16_t WaterMusData_ptr;
    uint16_t EndOfCastleMusData_ptr;
    uint16_t VictoryMusData_ptr;
    uint16_t EndOfCastleMusicEnvData_ptr;
    uint16_t AreaMusicEnvData_ptr;
    uint16_t WaterEventMusEnvData_ptr;
    uint16_t BowserFlameEnvData_ptr;
    uint16_t freeSpaceAddress;

    SMBDataPointers()
    {
        this->WSelectBufferTemplate_ptr = 0x8028;
        this->GameText_ptr = 0x80a1;
        this->TopStatusBarLine_ptr = 0x80a1;
        this->WorldLivesDisplay_ptr = 0x80c8;
        this->TwoPlayerTimeUp_ptr = 0x80e7;
        this->OnePlayerTimeUp_ptr = 0x80ef;
        this->TwoPlayerGameOver_ptr = 0x80fa;
        this->OnePlayerGameOver_ptr = 0x8102;
        this->WarpZoneWelcome_ptr = 0x810f;
        this->WarpZoneNumbers_ptr = 0x8141;
        this->Palette0_MTiles_ptr = 0x8191;
        this->Palette1_MTiles_ptr = 0x822d;
        this->Palette2_MTiles_ptr = 0x82e5;
        this->Palette3_MTiles_ptr = 0x830d;
        this->WaterPaletteData_ptr = 0x8325;
        this->GroundPaletteData_ptr = 0x8349;
        this->UndergroundPaletteData_ptr = 0x836d;
        this->CastlePaletteData_ptr = 0x8391;
        this->DaySnowPaletteData_ptr = 0x83b5;
        this->NightSnowPaletteData_ptr = 0x83bd;
        this->MushroomPaletteData_ptr = 0x83c5;
        this->BowserPaletteData_ptr = 0x83cd;
        this->MarioThanksMessage_ptr = 0x83d5;
        this->LuigiThanksMessage_ptr = 0x83e9;
        this->MushroomRetainerSaved_ptr = 0x83fd;
        this->PrincessSaved1_ptr = 0x8429;
        this->PrincessSaved2_ptr = 0x8440;
        this->WorldSelectMessage1_ptr = 0x845f;
        this->WorldSelectMessage2_ptr = 0x8470;
        this->PlayerBGPriorityData_ptr = 0x84bf;
        this->GameTimerData_ptr = 0x84c7;
        this->VerticalPipeData_ptr = 0x862d;
        this->SolidBlockMetatiles_ptr = 0x863f;
        this->BrickMetatiles_ptr = 0x8643;
        this->WorldAddrOffsets_ptr = 0x866d;
        this->AreaAddrOffsets_ptr = 0x8675;
        this->World1Areas_ptr = 0x8675;
        this->World2Areas_ptr = 0x867a;
        this->World3Areas_ptr = 0x867f;
        this->World4Areas_ptr = 0x8683;
        this->World5Areas_ptr = 0x8688;
        this->World6Areas_ptr = 0x868c;
        this->World7Areas_ptr = 0x8690;
        this->World8Areas_ptr = 0x8695;
        this->E_CastleArea1_ptr = 0x8729;
        this->E_CastleArea2_ptr = 0x8750;
        this->E_CastleArea3_ptr = 0x8769;
        this->E_CastleArea4_ptr = 0x8798;
        this->E_CastleArea5_ptr = 0x87c3;
        this->E_CastleArea6_ptr = 0x87d8;
        this->E_GroundArea1_ptr = 0x8812;
        this->E_GroundArea2_ptr = 0x8837;
        this->E_GroundArea3_ptr = 0x8854;
        this->E_GroundArea4_ptr = 0x8862;
        this->E_GroundArea5_ptr = 0x8889;
        this->E_GroundArea6_ptr = 0x88ba;
        this->E_GroundArea7_ptr = 0x88d8;
        this->E_GroundArea8_ptr = 0x88f5;
        this->E_GroundArea9_ptr = 0x890a;
        this->E_GroundArea10_ptr = 0x8934;
        this->E_GroundArea11_ptr = 0x8935;
        this->E_GroundArea12_ptr = 0x8959;
        this->E_GroundArea13_ptr = 0x8962;
        this->E_GroundArea14_ptr = 0x8987;
        this->E_GroundArea15_ptr = 0x89aa;
        this->E_GroundArea16_ptr = 0x89b3;
        this->E_GroundArea17_ptr = 0x89b4;
        this->E_GroundArea18_ptr = 0x89ee;
        this->E_GroundArea19_ptr = 0x8a19;
        this->E_GroundArea20_ptr = 0x8a47;
        this->E_GroundArea21_ptr = 0x8a63;
        this->E_GroundArea22_ptr = 0x8a6c;
        this->E_UndergroundArea1_ptr = 0x8a91;
        this->E_UndergroundArea2_ptr = 0x8abe;
        this->E_UndergroundArea3_ptr = 0x8aec;
        this->E_WaterArea1_ptr = 0x8b19;
        this->E_WaterArea2_ptr = 0x8b2a;
        this->E_WaterArea3_ptr = 0x8b54;
        this->L_CastleArea1_ptr = 0x8b68;
        this->L_CastleArea2_ptr = 0x8bc9;
        this->L_CastleArea3_ptr = 0x8c48;
        this->L_CastleArea4_ptr = 0x8cbb;
        this->L_CastleArea5_ptr = 0x8d28;
        this->L_CastleArea6_ptr = 0x8db3;
        this->L_GroundArea1_ptr = 0x8e24;
        this->L_GroundArea2_ptr = 0x8e87;
        this->L_GroundArea3_ptr = 0x8ef0;
        this->L_GroundArea4_ptr = 0x8f43;
        this->L_GroundArea5_ptr = 0x8fd2;
        this->L_GroundArea6_ptr = 0x9047;
        this->L_GroundArea7_ptr = 0x90ac;
        this->L_GroundArea8_ptr = 0x9101;
        this->L_GroundArea9_ptr = 0x9186;
        this->L_GroundArea10_ptr = 0x91eb;
        this->L_GroundArea11_ptr = 0x91f4;
        this->L_GroundArea12_ptr = 0x9233;
        this->L_GroundArea13_ptr = 0x9248;
        this->L_GroundArea14_ptr = 0x92af;
        this->L_GroundArea15_ptr = 0x9314;
        this->L_GroundArea16_ptr = 0x9387;
        this->L_GroundArea17_ptr = 0x93b8;
        this->L_GroundArea18_ptr = 0x944b;
        this->L_GroundArea19_ptr = 0x94be;
        this->L_GroundArea20_ptr = 0x9537;
        this->L_GroundArea21_ptr = 0x9590;
        this->L_GroundArea22_ptr = 0x95bb;
        this->L_UndergroundArea1_ptr = 0x95ee;
        this->L_UndergroundArea2_ptr = 0x9691;
        this->L_UndergroundArea3_ptr = 0x9732;
        this->L_WaterArea1_ptr = 0x97bf;
        this->L_WaterArea2_ptr = 0x97fe;
        this->L_WaterArea3_ptr = 0x9879;
        // Moved out of the packed data below, into the free space that starts at
        // freeSpaceAddress. The game can index this one 255 bytes past its end,
        // so the whole 256 bytes it reaches are reserved for it, up to $a5f8.
        // See loadConstantData() in SMBData.cpp.
        this->FireballXSpdData_ptr = 0xa4f9;
        // Moved out of the packed data below, as one blob holding every byte the
        // game can index the two of them with. See loadConstantData() in
        // SMBData.cpp.
        this->Bubble_MForceData_ptr = 0xa5ff;
        this->BubbleTimerData_ptr = 0xa601;
        this->BrickQBlockMetatiles_ptr = 0x9905;
        this->FlameYPosData_ptr = 0x9970;
        this->Bitmasks_ptr = 0x9982;
        this->FirebarMirrorData_ptr = 0x9a0f;
        this->FirebarTblOffsets_ptr = 0x9a13;
        this->FirebarYPos_ptr = 0x9a1f;
        // Both moved out of the packed data below, as blobs holding every byte the
        // game can index them with. See loadConstantData() in SMBData.cpp.
        this->LakituDiffAdj_ptr = 0x9a2b;
        this->BridgeCollapseData_ptr = 0x9a2e;
        this->ResidualXSpdData_ptr = 0x9a5d;
        // Moved out of the packed data below, together and in the order the ROM
        // has them, with a byte to spare on either side. See loadConstantData()
        // in SMBData.cpp.
        this->ClimbXPosAdder_ptr = 0xa5fa;
        this->ClimbPLocAdder_ptr = 0xa5fc;
        this->PlayerGfxTblOffsets_ptr = 0x9c8a;
        this->PlayerGraphicsTable_ptr = 0x9c9a;
        this->XOffscreenBitsData_ptr = 0x9d89;
        this->DefaultXOnscreenOfs_ptr = 0x9d99;
        this->MusicHeaderData_ptr = 0x9e0c;
        this->TimeRunningOutHdr_ptr = 0x9e3d;
        this->Star_CloudHdr_ptr = 0x9e42;
        this->EndOfLevelMusHdr_ptr = 0x9e48;
        this->ResidualHeaderData_ptr = 0x9e4d;
        this->UndergroundMusHdr_ptr = 0x9e52;
        this->SilenceHdr_ptr = 0x9e57;
        this->CastleMusHdr_ptr = 0x9e5b;
        this->VictoryMusHdr_ptr = 0x9e60;
        this->GameOverMusHdr_ptr = 0x9e65;
        this->WaterMusHdr_ptr = 0x9e6a;
        this->WinCastleMusHdr_ptr = 0x9e70;
        this->GroundLevelPart1Hdr_ptr = 0x9e75;
        this->GroundLevelPart2AHdr_ptr = 0x9e7b;
        this->GroundLevelPart2BHdr_ptr = 0x9e81;
        this->GroundLevelPart2CHdr_ptr = 0x9e87;
        this->GroundLevelPart3AHdr_ptr = 0x9e8d;
        this->GroundLevelPart3BHdr_ptr = 0x9e93;
        this->GroundLevelLeadInHdr_ptr = 0x9e99;
        this->GroundLevelPart4AHdr_ptr = 0x9e9f;
        this->GroundLevelPart4BHdr_ptr = 0x9ea5;
        this->GroundLevelPart4CHdr_ptr = 0x9eab;
        this->DeathMusHdr_ptr = 0x9eb1;
        this->Star_CloudMData_ptr = 0x9eb7;
        this->GroundM_P1Data_ptr = 0x9f00;
        this->SilenceData_ptr = 0x9f1b;
        this->GroundM_P2AData_ptr = 0x9f48;
        this->GroundM_P2BData_ptr = 0x9f74;
        this->GroundM_P2CData_ptr = 0x9f9c;
        this->GroundM_P3AData_ptr = 0x9fc1;
        this->GroundM_P3BData_ptr = 0x9fda;
        this->GroundMLdInData_ptr = 0x9ff8;
        this->GroundM_P4AData_ptr = 0xa024;
        this->GroundM_P4BData_ptr = 0xa04a;
        this->DeathMusData_ptr = 0xa071;
        this->GroundM_P4CData_ptr = 0xa073;
        this->CastleMusData_ptr = 0xa0a3;
        this->GameOverMusData_ptr = 0xa144;
        this->TimeRunOutMusData_ptr = 0xa171;
        this->WinLevelMusData_ptr = 0xa1af;
        this->UndergroundMusData_ptr = 0xa210;
        this->WaterMusData_ptr = 0xa251;
        this->EndOfCastleMusData_ptr = 0xa350;
        this->VictoryMusData_ptr = 0xa3c7;
        this->EndOfCastleMusicEnvData_ptr = 0xa495;
        this->AreaMusicEnvData_ptr = 0xa499;
        this->WaterEventMusEnvData_ptr = 0xa4a1;
        this->BowserFlameEnvData_ptr = 0xa4c9;
        this->freeSpaceAddress = 0xa4f9;
    }
};

// Defines for quick access of the addresses within SMBDataPointers
//

#define WSelectBufferTemplate (dataPointers.WSelectBufferTemplate_ptr)
#define GameText (dataPointers.GameText_ptr)
#define TopStatusBarLine (dataPointers.TopStatusBarLine_ptr)
#define WorldLivesDisplay (dataPointers.WorldLivesDisplay_ptr)
#define TwoPlayerTimeUp (dataPointers.TwoPlayerTimeUp_ptr)
#define OnePlayerTimeUp (dataPointers.OnePlayerTimeUp_ptr)
#define TwoPlayerGameOver (dataPointers.TwoPlayerGameOver_ptr)
#define OnePlayerGameOver (dataPointers.OnePlayerGameOver_ptr)
#define WarpZoneWelcome (dataPointers.WarpZoneWelcome_ptr)
#define WarpZoneNumbers (dataPointers.WarpZoneNumbers_ptr)
#define Palette0_MTiles (dataPointers.Palette0_MTiles_ptr)
#define Palette1_MTiles (dataPointers.Palette1_MTiles_ptr)
#define Palette2_MTiles (dataPointers.Palette2_MTiles_ptr)
#define Palette3_MTiles (dataPointers.Palette3_MTiles_ptr)
#define WaterPaletteData (dataPointers.WaterPaletteData_ptr)
#define GroundPaletteData (dataPointers.GroundPaletteData_ptr)
#define UndergroundPaletteData (dataPointers.UndergroundPaletteData_ptr)
#define CastlePaletteData (dataPointers.CastlePaletteData_ptr)
#define DaySnowPaletteData (dataPointers.DaySnowPaletteData_ptr)
#define NightSnowPaletteData (dataPointers.NightSnowPaletteData_ptr)
#define MushroomPaletteData (dataPointers.MushroomPaletteData_ptr)
#define BowserPaletteData (dataPointers.BowserPaletteData_ptr)
#define MarioThanksMessage (dataPointers.MarioThanksMessage_ptr)
#define LuigiThanksMessage (dataPointers.LuigiThanksMessage_ptr)
#define MushroomRetainerSaved (dataPointers.MushroomRetainerSaved_ptr)
#define PrincessSaved1 (dataPointers.PrincessSaved1_ptr)
#define PrincessSaved2 (dataPointers.PrincessSaved2_ptr)
#define WorldSelectMessage1 (dataPointers.WorldSelectMessage1_ptr)
#define WorldSelectMessage2 (dataPointers.WorldSelectMessage2_ptr)
#define PlayerBGPriorityData (dataPointers.PlayerBGPriorityData_ptr)
#define GameTimerData (dataPointers.GameTimerData_ptr)
#define VerticalPipeData (dataPointers.VerticalPipeData_ptr)
#define SolidBlockMetatiles (dataPointers.SolidBlockMetatiles_ptr)
#define BrickMetatiles (dataPointers.BrickMetatiles_ptr)
#define WorldAddrOffsets (dataPointers.WorldAddrOffsets_ptr)
#define AreaAddrOffsets (dataPointers.AreaAddrOffsets_ptr)
#define World1Areas (dataPointers.World1Areas_ptr)
#define World2Areas (dataPointers.World2Areas_ptr)
#define World3Areas (dataPointers.World3Areas_ptr)
#define World4Areas (dataPointers.World4Areas_ptr)
#define World5Areas (dataPointers.World5Areas_ptr)
#define World6Areas (dataPointers.World6Areas_ptr)
#define World7Areas (dataPointers.World7Areas_ptr)
#define World8Areas (dataPointers.World8Areas_ptr)
#define E_CastleArea1 (dataPointers.E_CastleArea1_ptr)
#define E_CastleArea2 (dataPointers.E_CastleArea2_ptr)
#define E_CastleArea3 (dataPointers.E_CastleArea3_ptr)
#define E_CastleArea4 (dataPointers.E_CastleArea4_ptr)
#define E_CastleArea5 (dataPointers.E_CastleArea5_ptr)
#define E_CastleArea6 (dataPointers.E_CastleArea6_ptr)
#define E_GroundArea1 (dataPointers.E_GroundArea1_ptr)
#define E_GroundArea2 (dataPointers.E_GroundArea2_ptr)
#define E_GroundArea3 (dataPointers.E_GroundArea3_ptr)
#define E_GroundArea4 (dataPointers.E_GroundArea4_ptr)
#define E_GroundArea5 (dataPointers.E_GroundArea5_ptr)
#define E_GroundArea6 (dataPointers.E_GroundArea6_ptr)
#define E_GroundArea7 (dataPointers.E_GroundArea7_ptr)
#define E_GroundArea8 (dataPointers.E_GroundArea8_ptr)
#define E_GroundArea9 (dataPointers.E_GroundArea9_ptr)
#define E_GroundArea10 (dataPointers.E_GroundArea10_ptr)
#define E_GroundArea11 (dataPointers.E_GroundArea11_ptr)
#define E_GroundArea12 (dataPointers.E_GroundArea12_ptr)
#define E_GroundArea13 (dataPointers.E_GroundArea13_ptr)
#define E_GroundArea14 (dataPointers.E_GroundArea14_ptr)
#define E_GroundArea15 (dataPointers.E_GroundArea15_ptr)
#define E_GroundArea16 (dataPointers.E_GroundArea16_ptr)
#define E_GroundArea17 (dataPointers.E_GroundArea17_ptr)
#define E_GroundArea18 (dataPointers.E_GroundArea18_ptr)
#define E_GroundArea19 (dataPointers.E_GroundArea19_ptr)
#define E_GroundArea20 (dataPointers.E_GroundArea20_ptr)
#define E_GroundArea21 (dataPointers.E_GroundArea21_ptr)
#define E_GroundArea22 (dataPointers.E_GroundArea22_ptr)
#define E_UndergroundArea1 (dataPointers.E_UndergroundArea1_ptr)
#define E_UndergroundArea2 (dataPointers.E_UndergroundArea2_ptr)
#define E_UndergroundArea3 (dataPointers.E_UndergroundArea3_ptr)
#define E_WaterArea1 (dataPointers.E_WaterArea1_ptr)
#define E_WaterArea2 (dataPointers.E_WaterArea2_ptr)
#define E_WaterArea3 (dataPointers.E_WaterArea3_ptr)
#define L_CastleArea1 (dataPointers.L_CastleArea1_ptr)
#define L_CastleArea2 (dataPointers.L_CastleArea2_ptr)
#define L_CastleArea3 (dataPointers.L_CastleArea3_ptr)
#define L_CastleArea4 (dataPointers.L_CastleArea4_ptr)
#define L_CastleArea5 (dataPointers.L_CastleArea5_ptr)
#define L_CastleArea6 (dataPointers.L_CastleArea6_ptr)
#define L_GroundArea1 (dataPointers.L_GroundArea1_ptr)
#define L_GroundArea2 (dataPointers.L_GroundArea2_ptr)
#define L_GroundArea3 (dataPointers.L_GroundArea3_ptr)
#define L_GroundArea4 (dataPointers.L_GroundArea4_ptr)
#define L_GroundArea5 (dataPointers.L_GroundArea5_ptr)
#define L_GroundArea6 (dataPointers.L_GroundArea6_ptr)
#define L_GroundArea7 (dataPointers.L_GroundArea7_ptr)
#define L_GroundArea8 (dataPointers.L_GroundArea8_ptr)
#define L_GroundArea9 (dataPointers.L_GroundArea9_ptr)
#define L_GroundArea10 (dataPointers.L_GroundArea10_ptr)
#define L_GroundArea11 (dataPointers.L_GroundArea11_ptr)
#define L_GroundArea12 (dataPointers.L_GroundArea12_ptr)
#define L_GroundArea13 (dataPointers.L_GroundArea13_ptr)
#define L_GroundArea14 (dataPointers.L_GroundArea14_ptr)
#define L_GroundArea15 (dataPointers.L_GroundArea15_ptr)
#define L_GroundArea16 (dataPointers.L_GroundArea16_ptr)
#define L_GroundArea17 (dataPointers.L_GroundArea17_ptr)
#define L_GroundArea18 (dataPointers.L_GroundArea18_ptr)
#define L_GroundArea19 (dataPointers.L_GroundArea19_ptr)
#define L_GroundArea20 (dataPointers.L_GroundArea20_ptr)
#define L_GroundArea21 (dataPointers.L_GroundArea21_ptr)
#define L_GroundArea22 (dataPointers.L_GroundArea22_ptr)
#define L_UndergroundArea1 (dataPointers.L_UndergroundArea1_ptr)
#define L_UndergroundArea2 (dataPointers.L_UndergroundArea2_ptr)
#define L_UndergroundArea3 (dataPointers.L_UndergroundArea3_ptr)
#define L_WaterArea1 (dataPointers.L_WaterArea1_ptr)
#define L_WaterArea2 (dataPointers.L_WaterArea2_ptr)
#define L_WaterArea3 (dataPointers.L_WaterArea3_ptr)
#define FireballXSpdData (dataPointers.FireballXSpdData_ptr)
#define Bubble_MForceData (dataPointers.Bubble_MForceData_ptr)
#define BubbleTimerData (dataPointers.BubbleTimerData_ptr)
#define BrickQBlockMetatiles (dataPointers.BrickQBlockMetatiles_ptr)
#define FlameYPosData (dataPointers.FlameYPosData_ptr)
#define Bitmasks (dataPointers.Bitmasks_ptr)
#define FirebarMirrorData (dataPointers.FirebarMirrorData_ptr)
#define FirebarTblOffsets (dataPointers.FirebarTblOffsets_ptr)
#define FirebarYPos (dataPointers.FirebarYPos_ptr)
#define LakituDiffAdj (dataPointers.LakituDiffAdj_ptr)
#define BridgeCollapseData (dataPointers.BridgeCollapseData_ptr)
#define ResidualXSpdData (dataPointers.ResidualXSpdData_ptr)
#define ClimbXPosAdder (dataPointers.ClimbXPosAdder_ptr)
#define ClimbPLocAdder (dataPointers.ClimbPLocAdder_ptr)
#define PlayerGfxTblOffsets (dataPointers.PlayerGfxTblOffsets_ptr)
#define PlayerGraphicsTable (dataPointers.PlayerGraphicsTable_ptr)
#define XOffscreenBitsData (dataPointers.XOffscreenBitsData_ptr)
#define DefaultXOnscreenOfs (dataPointers.DefaultXOnscreenOfs_ptr)
#define MusicHeaderData (dataPointers.MusicHeaderData_ptr)
#define TimeRunningOutHdr (dataPointers.TimeRunningOutHdr_ptr)
#define Star_CloudHdr (dataPointers.Star_CloudHdr_ptr)
#define EndOfLevelMusHdr (dataPointers.EndOfLevelMusHdr_ptr)
#define ResidualHeaderData (dataPointers.ResidualHeaderData_ptr)
#define UndergroundMusHdr (dataPointers.UndergroundMusHdr_ptr)
#define SilenceHdr (dataPointers.SilenceHdr_ptr)
#define CastleMusHdr (dataPointers.CastleMusHdr_ptr)
#define VictoryMusHdr (dataPointers.VictoryMusHdr_ptr)
#define GameOverMusHdr (dataPointers.GameOverMusHdr_ptr)
#define WaterMusHdr (dataPointers.WaterMusHdr_ptr)
#define WinCastleMusHdr (dataPointers.WinCastleMusHdr_ptr)
#define GroundLevelPart1Hdr (dataPointers.GroundLevelPart1Hdr_ptr)
#define GroundLevelPart2AHdr (dataPointers.GroundLevelPart2AHdr_ptr)
#define GroundLevelPart2BHdr (dataPointers.GroundLevelPart2BHdr_ptr)
#define GroundLevelPart2CHdr (dataPointers.GroundLevelPart2CHdr_ptr)
#define GroundLevelPart3AHdr (dataPointers.GroundLevelPart3AHdr_ptr)
#define GroundLevelPart3BHdr (dataPointers.GroundLevelPart3BHdr_ptr)
#define GroundLevelLeadInHdr (dataPointers.GroundLevelLeadInHdr_ptr)
#define GroundLevelPart4AHdr (dataPointers.GroundLevelPart4AHdr_ptr)
#define GroundLevelPart4BHdr (dataPointers.GroundLevelPart4BHdr_ptr)
#define GroundLevelPart4CHdr (dataPointers.GroundLevelPart4CHdr_ptr)
#define DeathMusHdr (dataPointers.DeathMusHdr_ptr)
#define Star_CloudMData (dataPointers.Star_CloudMData_ptr)
#define GroundM_P1Data (dataPointers.GroundM_P1Data_ptr)
#define SilenceData (dataPointers.SilenceData_ptr)
#define GroundM_P2AData (dataPointers.GroundM_P2AData_ptr)
#define GroundM_P2BData (dataPointers.GroundM_P2BData_ptr)
#define GroundM_P2CData (dataPointers.GroundM_P2CData_ptr)
#define GroundM_P3AData (dataPointers.GroundM_P3AData_ptr)
#define GroundM_P3BData (dataPointers.GroundM_P3BData_ptr)
#define GroundMLdInData (dataPointers.GroundMLdInData_ptr)
#define GroundM_P4AData (dataPointers.GroundM_P4AData_ptr)
#define GroundM_P4BData (dataPointers.GroundM_P4BData_ptr)
#define DeathMusData (dataPointers.DeathMusData_ptr)
#define GroundM_P4CData (dataPointers.GroundM_P4CData_ptr)
#define CastleMusData (dataPointers.CastleMusData_ptr)
#define GameOverMusData (dataPointers.GameOverMusData_ptr)
#define TimeRunOutMusData (dataPointers.TimeRunOutMusData_ptr)
#define WinLevelMusData (dataPointers.WinLevelMusData_ptr)
#define UndergroundMusData (dataPointers.UndergroundMusData_ptr)
#define WaterMusData (dataPointers.WaterMusData_ptr)
#define EndOfCastleMusData (dataPointers.EndOfCastleMusData_ptr)
#define VictoryMusData (dataPointers.VictoryMusData_ptr)
#define EndOfCastleMusicEnvData (dataPointers.EndOfCastleMusicEnvData_ptr)
#define AreaMusicEnvData (dataPointers.AreaMusicEnvData_ptr)
#define WaterEventMusEnvData (dataPointers.WaterEventMusEnvData_ptr)
#define BowserFlameEnvData (dataPointers.BowserFlameEnvData_ptr)

#endif // SMBDATAPOINTERS_HPP
