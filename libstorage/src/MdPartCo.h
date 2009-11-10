/*
 * File: MdPartCo.cc
 *
 * Declaration of MdPartCo class which represents single MD Device (RAID
 * Volume) like md126 which is a Container for partitions.
 *
 * Copyright (c) 2009, Intel Corporation.
 * Copyright (c) 2009 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MD_PART_CO_H
#define MD_PART_CO_H

#include <list>

#include "y2storage/Container.h"
#include "y2storage/Disk.h"
#include "y2storage/MdPart.h"

namespace storage
{

class Storage;
class SystemCmd;
class ProcPart;
class Region;
class EtcRaidtab;

/**
 * Class: MdPartCo
 *
 * Brief:
 *  MD RAID Volume that can be partitioned and used as container for partitions.
*/
class MdPartCo : public Container
    {
    friend class Storage;

    public:
    MdPartCo( Storage * const s,
              const string& Name,
              ProcPart* ppart = NULL);

    MdPartCo( const MdPartCo& rhs );

    virtual ~MdPartCo();


    unsigned long long sizeK() const { return size_k; }
    const string& labelName() const { return disk->labelName(); }
    const string& udevPath() const { return udev_path; }
    const std::list<string>& udevId() const { return udev_id; }
    unsigned numPartitions() const { return disk->numPartitions(); }
    static storage::CType staticType() { return storage::MDPART; }
    friend std::ostream& operator<< (std::ostream&, const MdPartCo& );
    void setUdevData(const list<string>& id);

    /* Checks if name fits with MD name*/
    bool matchMdName(const string& name ) { return (name==nm); }

    void getMdPartCoState(storage::MdPartCoStateInfo& info);

    int createPartition( storage::PartitionType type, long unsigned start,
        long unsigned len, string& device,
        bool checkRelaxed=false );
    int createPartition( long unsigned len, string& device,
        bool checkRelaxed=false );
    int createPartition( storage::PartitionType type, string& device );
    int removePartition( unsigned nr );
    int changePartitionId( unsigned nr, unsigned id );
    int forgetChangePartitionId( unsigned nr );
    int changePartitionArea( unsigned nr, unsigned long start,
        unsigned long size, bool checkRelaxed=false );
    int nextFreePartition(storage::PartitionType type, unsigned& nr,
        string& device) const;
    int destroyPartitionTable( const string& new_label );
    int freeCylindersAfterPartition(const MdPart* p, unsigned long& freeCyls) const;
    int resizePartition( MdPart* p, unsigned long newCyl );
    int resizeVolume( Volume* v, unsigned long long newSize );
    int removeVolume( Volume* v );
    int removeMdPart();

    unsigned maxPrimary() const { return disk->maxPrimary(); }
    bool extendedPossible() const { return disk->extendedPossible(); }
    unsigned maxLogical() const { return disk->maxLogical(); }

    unsigned int numPrimary() const { return disk->numPrimary(); }
    bool hasExtended() const { return disk->hasExtended(); }
    unsigned int numLogical() const { return disk->numLogical(); }

    void getUnusedSpace(std::list<Region>& free, bool all = true, bool logical = false) const
    { disk->getUnusedSpace(free, all, logical); }

    unsigned long long cylinderToKb( unsigned long val ) const
    { return disk->cylinderToKb( val ); }
    unsigned long kbToCylinder( unsigned long long val ) const
    { return disk->kbToCylinder( val ); }
    string getPartName( unsigned nr ) const;

    virtual void getCommitActions( std::list<storage::commitAction*>& l ) const;
    virtual int getToCommit( storage::CommitStage stage,
        std::list<Container*>& col,
        std::list<Volume*>& vol );
    virtual int commitChanges( storage::CommitStage stage );
    int commitChanges( storage::CommitStage stage, Volume* vol );

    Partition* getPartition( unsigned nr, bool del );
    int getPartitionInfo(deque<storage::PartitionInfo>& plist);
    void getInfo( storage::MdPartCoInfo& info ) const;
    bool equalContent( const Container& rhs ) const;
    virtual string getDiffString( const Container& d ) const;
    void logDifference( const MdPartCo& d ) const;
    MdPartCo& operator= ( const MdPartCo& rhs );
    static string undevName( const string& name );
    string numToName( unsigned mdNum ) const;

    static list<string> getMdRaids();

    void syncRaidtab();

    int nr();
    /* RAID Related functionality */
    unsigned long chunkSize() const { return chunk_size; }

    storage::MdType personality() const { return md_type; }

    storage::MdArrayState getArrayState() { return md_state; };

    void getMdUuid( string&val ) { val=md_uuid; }

    /* Raid Level of the RAID as string. */
    const string& pName() const { return md_names[md_type]; }
    /* Parity for some of RAID's. */
    const string& ptName() const { return par_names[md_parity]; }
    /* Devices from which RAID is composed. */
    void getDevs( std::list<string>& devices, bool all=true, bool spare=false ) const;


    void getSpareDevs(std::list<string>& devices );

    /* Is MD device? */
    static bool matchMdRegex( const string& dev );
    /* MD Device major number. */
    static unsigned mdMajor();

    /* Raid Level as string for given type. */
    static const string& pName( storage::MdType t ) { return md_names[t]; }

    static void activate( bool val, const string& tmpDir  );

    static bool isActive( void  ) { return active; }

    /* Return true if on RAID Volume is partition table */
    static bool hasPartitionTable(const string& name );
    /* Return true if there is no Filesystem on device (it can contain partition table). */
    static bool hasFileSystem(const string& name);


    static void setHandlingDev(bool val) { handlingMd = val; }
    static bool isHandlingDev() { return handlingMd; }

    static bool isImsmPlatform();

    static bool matchRegex( const string& dev );
    static bool mdStringNum( const string& name, unsigned& num );

    // This function will scann for MD RAIDs and will return
    // list with detected RAID names.
    static int scanForRaid(list<string>& raidNames);

    /* filterMdPartCo
     * Get list of detected MD RAIDs and filters them for
     * those which can be handled by MdPartCo.
     */
    static list<string> filterMdPartCo(list<string>& raidList,
                                       ProcPart& ppart,
                                       bool isInst);


    protected:
    // iterators over partitions
    // protected typedefs for iterators over partitions
    typedef CastIterator<VIter, MdPart *> MdPartInter;
    typedef CastIterator<CVIter, const MdPart *> MdPartCInter;
    template< class Pred >
        struct MdPartPI { typedef ContainerIter<Pred, MdPartInter> type; };
    template< class Pred >
        struct MdPartCPI { typedef ContainerIter<Pred, MdPartCInter> type; };
    typedef CheckFnc<const MdPart> CheckFncMdPart;
    typedef CheckerIterator< CheckFncMdPart, MdPartPI<CheckFncMdPart>::type,
                             MdPartInter, MdPart > MdPartPIterator;
    typedef CheckerIterator< CheckFncMdPart, MdPartCPI<CheckFncMdPart>::type,
                             MdPartCInter, const MdPart > MdPartCPIterator;
    typedef DerefIterator<MdPartPIterator,MdPart> MdPartIter;
    typedef DerefIterator<MdPartCPIterator,const MdPart> ConstMdPartIter;
    typedef IterPair<MdPartIter> MdPartPair;
    typedef IterPair<ConstMdPartIter> ConstMdPartPair;

    MdPartPair mdpartPair( bool (* CheckMdPart)( const MdPart& )=NULL)
        {
        return( MdPartPair( mdpartBegin( CheckMdPart ), mdpartEnd( CheckMdPart ) ));
        }
    MdPartIter mdpartBegin( bool (* CheckMdPart)( const MdPart& )=NULL)
        {
        IterPair<MdPartInter> p( (MdPartInter(begin())), (MdPartInter(end())) );
        return( MdPartIter( MdPartPIterator( p, CheckMdPart )) );
        }
    MdPartIter mdpartEnd( bool (* CheckMdPart)( const MdPart& )=NULL)
        {
        IterPair<MdPartInter> p( (MdPartInter(begin())), (MdPartInter(end())) );
        return( MdPartIter( MdPartPIterator( p, CheckMdPart, true )) );
        }

    ConstMdPartPair mdpartPair( bool (* CheckMdPart)( const MdPart& )=NULL) const
        {
        return( ConstMdPartPair( mdpartBegin( CheckMdPart ), mdpartEnd( CheckMdPart ) ));
        }
    ConstMdPartIter mdpartBegin( bool (* CheckMdPart)( const MdPart& )=NULL) const
        {
        IterPair<MdPartCInter> p( (MdPartCInter(begin())), (MdPartCInter(end())) );
        return( ConstMdPartIter( MdPartCPIterator( p, CheckMdPart )) );
        }
    ConstMdPartIter mdpartEnd( bool (* CheckMdPart)( const MdPart& )=NULL) const
        {
        IterPair<MdPartCInter> p( (MdPartCInter(begin())), (MdPartCInter(end())) );
        return( ConstMdPartIter( MdPartCPIterator( p, CheckMdPart, true )) );
        }

    MdPartCo( Storage * const s, const string& File );
    virtual void print( std::ostream& s ) const { s << *this; }
    virtual Container* getCopy() const { return( new MdPartCo( *this ) ); }
    void activate_part( bool val );
    void init( ProcPart* ppart );
    void createDisk( ProcPart* ppart );
    void getVolumes( ProcPart* ppart );
    void updatePointers( bool invalid=false );
    void updateMinor();
    virtual void newP( MdPart*& dm, unsigned num, Partition* p );
    int addNewDev( string& device );
    int updateDelDev();
    void handleWholeDevice();
    void removeFromMemory();
    void removePresentPartitions();
    bool validPartition( const Partition* p );
    bool findMdPart( unsigned nr, MdPartIter& i );

    void updateEntry();
    string mdadmLine() const;
    void raidtabLines( list<string>& lines ) const;


    static bool partNotDeleted( const MdPart&d ) { return( !d.deleted() ); }

    int doCreate( Volume* v );
    int doRemove( Volume* v );
    int doResize( Volume* v );
    int doSetType( MdPart* v );
    int doCreateLabel();
    virtual int doRemove();
    virtual string removeText( bool doing ) const;
    virtual string setDiskLabelText( bool doing ) const;

    void getMajorMinor(void);

    /* Makes sure that dev=/dev/name is name doesn't contains this prefix*/
    void makeDevName(const string& name );
    /* Initialize the MD part of object.*/
    void initMd(void);

    void setSize(unsigned long long size );
    /* */
    static bool isMdName(const string& name);

    bool isMdPart(const string& name);

    void getPartNum(const string& device, unsigned& num);

    void getMdProps(void);

    void setSpares(void);

    void logData( const string& Dir );
    string udev_path;
    std::list<string> udev_id;
    string logfile_name;

    Disk* disk;
    bool del_ptable;
    unsigned num_part;

    /* RAID Related */

    /* Returns container */
    void getParent();

    void setMetaData();

    void setMdDevs();

    void setMdParity();

    /* returns devices listed as slaves in sysfs directory */
    void getSlaves(const string name, std::list<string>& devs_list );

    /* fields in 'map' file */
    enum mdMap { MAP_DEV=0, MAP_META, MAP_UUID, MAP_NAME, };
    bool findMdMap(std::ifstream& file);
    bool readMdMap();


    //Input: 'mdXXX' device.
    static storage::CType envSelection(const string& name);
    static bool havePartsInProc(const string& name, ProcPart& ppart);

    static void getMdMajor();
    static storage::MdType toMdType( const string& val );
    static storage::MdParity toMdParity( const string& val );
    static storage::MdArrayState toMdArrayState( const string& val );



    unsigned long chunk_size;
    storage::MdType md_type;
    storage::MdParity md_parity;
    storage::MdArrayState md_state;

    /* Md Container - */
    string parent_container;
    string md_metadata;
    string md_uuid;
    string sb_ver;
    bool destrSb;
    std::list<string> devs;
    std::list<string> spare;
    static string md_names[storage::MULTIPATH+1];
    static string par_names[storage::RIGHT_SYMMETRIC+1];
    static string md_states[storage::ACTIVE_IDLE+1];
    static unsigned md_major;

    /* Name that is present in /dev/md directory.*/
    string md_name;

    static string sysfs_path;

    enum MdProperty
    {
      METADATA=0,
      COMPONENT_SIZE,
      CHUNK_SIZE,
      ARRAY_STATE,
      LEVEL,
      LAYOUT,
      /* ... */
      MDPROP_LAST,
    };
    static string md_props[MDPROP_LAST];

    bool readProp(enum MdProperty prop, string& val);

    /* For that RAID type parity means something */
    bool hasParity() const
    { return md_type == RAID5 || md_type == RAID6 || md_type == RAID10; }

    mutable storage::MdPartCoInfo info;

    static bool active;
    static bool handlingMd;
    };
}

#endif