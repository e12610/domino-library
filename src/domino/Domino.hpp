/**
 * Copyright 2017 Nokia. All rights reserved.
 */
// ***********************************************************************************************
// - what:
//   . like eNB upgrade, many clues, many tasks, how to align the mass to best performance & tidy?
//   . treat each clue as an event, like a domino tile
//   . treat each task as a hdlr (callback func)
//   . each hdlr relies on limited event(s), each event may also rely on some ohter event(s)
//   . whenever an event occurred, following event(s) can be auto-triggered, so on calling hdlr(s)
//   . this will go till end (like domino)
// - why:
//   . global states (like global_var) via eg ObjAnywhere
//   * easily bind & auto broadcast (like real dominos)
//     * auto shape in different scenario
//   * easily link hdlr with these states & auto callback
//     . conclusive callback instead of trigger
//     * can min hdlr's conditions & call as-early-as possible
//     . easy adapt change since each hdlr/event has limited pre-condition, auto impact others
//   * light weight (better than IM)
//   * need not base-derive classes, eg RuAgent/SmodAgent need not derive from BaseAgent
//     . 1 set hdlrs with unique NDL/precheck/RFM/RB/FB domino set
// - assmuption:
//   . each event-hdlr is called only when event state F->T
//   . repeated event/hdlr is complex, be careful(eg DominoTests.newTriggerViaChain)
//   . dead-loop: complex so not code check but may tool check (eg excel)
// - core: states_
// - VALUE:
//   * auto broadcast, auto callback, auto shape [MUST-HAVE!]
//   . share state & hdlr
//   . EvName index
//   . template extension (PriDomino, etc)
// ***********************************************************************************************
#ifndef DOMINO_HPP_
#define DOMINO_HPP_

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "CppLog.hpp"

namespace RLib
{
// ***********************************************************************************************
class Domino
{
public:
    using Event      = size_t;  // smaller size can save mem; larger size can support more events
    using Events     = std::set<Event>;
    using EvName     = std::string;
    using SimuEvents = std::map<EvName, bool>;  // not unordered-map since most traversal

    enum
    {
        N_EVENT_STATE      = 2,
        D_EVENT_FAILED_RET = static_cast<size_t>(-1),
    };

    // -------------------------------------------------------------------------------------------
    // Each Tile in Domino is a record, containing:
    // - Event:  tile's internal ID  , mandatory
    // - EvName: tile's external ID  , mandatory
    // - state:  tile's up/down state, mandatory, default=false
    // - prev:   prev tile(s)        , optional
    // -------------------------------------------------------------------------------------------
    Domino() : log_("Dmn-" + std::to_string(dmnID_++)) {}
    virtual ~Domino() = default;

    Event newEvent(const EvName&);
    Event getEventBy(const EvName&) const;

    bool   state(const EvName& aEvName) const { return state(getEventBy(aEvName)); }
    void   setState(const SimuEvents&);
    Event  setPrev(const EvName&, const SimuEvents& aSimuPrevEvents);
    EvName whyFalse(const EvName&) const;

    // -------------------------------------------------------------------------------------------
    // misc:
    size_t nEvent() const { return states_.size(); }

protected:
    const EvName& evName(const Event aEv) const { return evNames_[aEv]; }  // aEv must valid
    bool state(const Event aEv) const { return aEv < states_.size() ? states_[aEv] : false; }
    virtual void effect(const Event) {}

private:
    void deduceState(const Event);
    void pureSetState(const Event, const bool aNewState);

    // -------------------------------------------------------------------------------------------
    std::vector<bool> states_;                     // bitmap & dyn expand, [event]=t/f

    std::map<Event, Events> prev_[N_EVENT_STATE];  // not unordered-map since most traversal
    std::map<Event, Events> next_[N_EVENT_STATE];  // not unordered-map since most traversal
    std::unordered_map<EvName, Event> events_;     // [evName]=event
    std::vector<EvName> evNames_;                  // [event]=evName for easy debug
    bool sthChanged_ = false;                      // for debug

    static size_t dmnID_;
    static const EvName invalidEvName;

public:  // no impact self but convient non-member-func eg getValue() for DataDomino
    CppLog log_;
};

}  // namespace
#endif  // DOMINO_HPP_
// ***********************************************************************************************
// - where:
//   . start using domino for time-cost events
//   . 99% for intra-process (while can support inter-process)
//   . Ltd. - limit duty
// - why Event is not a class?
//   . states_ is better as bitmap, prev_/next_/hdlrs_/priorities_ may not exist for a event
//   . easier BasicDomino, PriDomino, etc
//   . no need new/share_ptr the Event class
//   . only 1 class(Domino) is for all access
// - why not throw exception?
//   . wider usage (eg moam not allow exception)
//   . same simple code without exception
// - why template PriDomino, etc
//   . easy combine: eg Basic + Pri + Rw or Basic + Rw or Basic + Pri
//   . direct to get interface of all combined classes (inherit)
// - why not separate EvNameDomino from BasicDomino?
//   . BasicDomino's debug needs EvNameDomino
// - why not separate SimuDomino from BasicDomino?
//   . tight couple at setState()
// - why not rm(event)
//   . dangeous: may break deduced path
//   . not must-have req
// - why Event:EvName=1:1?
//   . simplify complex scenario eg rmOneHdlrOK() may relate with multi-hdlr
//   . simplify interface: EvName only, Event is internal-use only
// - why rm "#define PRI_DOMINO (ObjAnywhere::get<PriDomino<Domino> >())"
//   . PriDomino<Domino> != PriDomino<DataDomino<Domino> >, ObjAnywhere::get() may ret null
// - why no nHdlr() & hasHdlr()
//   . MultiHdlrDomino also need to support them
//   . UT shall not check them but check cb - more direct
// - why must EvName on ALL interface?
//   . EvName is much safer than Event (random Event could be valid, but rare of random EvName)
//     . read-only interface can use Event; mislead? better forbid also?
//   . EvName is lower performance than Event?
//     . a little, hope compiler optimize it
//   . Can buffer last EvName ptr to speedup?
//     . dangeous: diff func could create EvName at same address in stack
//     . 021-09-22: all UT, only 41% getEventBy() can benefit by buffer, not worth vs dangeous
// - how:
//   *)trigger
//     . prefer time-cost events
//     . how serial coding while parallel running
//       1) prepare events/tiles step by step
//       2) when all done, let DominoEvents start execution
//     . the more complex, the more benefit
//   *)mobile: any code can be called via Domino from anywhere
//     . cloud vs tree (intra-process)
//     . multi-cloud vs 1 bigger tree (inter-process)
//     . share data
//   .)replace state machine
//     . more direct
//     . no waste code for each state
//   .)msgSelf instead of callback directly
//     . avoid strange bugs
//     . serial coding/reading but parallel executing
//     . can also support callback, but not must-have, & complex code/logic
// - suggestion:
//   . use domino only when better than no-use
//   . event hdlr shall cover succ, fail, not needed, etc all cases, to ease following code
// - todo:
//   . limit-write event to owner
//   . micro-service of PriDomino, RwDomino
//   . connect micro-service?
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2017-01-04  CSZ       1)create EventDriver
// 2017-02-10  CSZ       - multi-hdlrs
// 2017-02-13  CSZ       2)only false to true can call hdlr
// 2017-02-16  CSZ       - event key
// 2017-05-07  CSZ       - hdlr always via msgSelf (no direct call)
// 2018-01-01  CSZ       3)rename to Domino, whom() to prev()/next()
// 2018-02-24  CSZ       - separate Domino to Basic, Key, Priority, ReadWrite, etc
// 2018-02-28  CSZ       - SimuDomino
// 2018-08-16  CSZ       - rm throw-exception
// 2018-09-18  CSZ       - support repeated event/hdlr
// 2018-10-30  CSZ       - simultaneous setPrev()
// 2018-10-31  CSZ       4)Event:EvName=1:1(simpler); Event=internal, EvName=external
// 2019-11-11  CSZ       - debug(rm/hide/nothingChanged)
//                       5)domino is practical; waiting formal & larger practice
// 2020-02-06  CSZ       - whyFalse(EvName)
// 2020-04-03  CSZ       - contain msgSelf_ so multi Domino can be used simulteneously
// 2020-06-10  CSZ       - dmnID_ to identify different domino instance
// 2020-08-21  CSZ       - most map to unordered_map for search performance
// 2020-11-17  CSZ       - multi-hdlr from Domino to MultiHdlrDomino since single hdlr is enough
//                         for most Domino usecases
// 2020-11-26  CSZ       - SmartLog for UT
//                       + DOMINO return full nested dominos
//                       + minimize derived dominos (eg FreeHdlrDomino is only pureRmHdlrOK() in triggerHdlr())
//                       - minimize all dominos' mem func
//                       - ref instead of cp in all mem func
//                       - EvName for outer interface, Event for inner
//                       - priority req/ut first
// 2020-12-11  CSZ       6)separate hdlr from Domino to HdlrDomino
// 2021-03-24  CSZ       - auto&& for-loop
//                       - auto&& var = func()
//                       - ref ret of getEvNameBy()
// 2021-04-01  CSZ       - coding req
// 2022-01-17  PJ & CSZ  - formal log & naming
// ***********************************************************************************************
