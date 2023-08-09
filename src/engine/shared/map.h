/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_MAP_H
#define ENGINE_SHARED_MAP_H

#include <base/system.h>

#include "datafile.h"
#include <engine/map.h>

class CMap : public IEngineMap
{
	CDataFileReader m_DataFile;

public:
	CMap();

	CDataFileReader *GetReader() { return &m_DataFile; }

	void *GetData(int Index) override;
	int GetDataSize(int Index) const override;
	void *GetDataSwapped(int Index) override;
	void UnloadData(int Index) override;
	int NumData() const override;

	void *GetItem(int Index, int *pType = nullptr, int *pID = nullptr) override;
	int GetItemSize(int Index) override;
	void GetType(int Type, int *pStart, int *pNum) override;
	void *FindItem(int Type, int ID) override;
	int NumItems() const override;

	bool Load(const char *pMapName) override;
	void Unload() override;
	bool IsLoaded() const override;
	IOHANDLE File() const override;

	SHA256_DIGEST Sha256() const override;
	unsigned Crc() const override;
	int MapSize() const override;

	static void ExtractTiles(class CTile *pDest, size_t DestSize, const class CTile *pSrc, size_t SrcSize);
};

#endif
