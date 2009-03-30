#include "playdar/application.h"
#include "playdar/rs_local_library.h"
#include "playdar/library.h"
#include <boost/foreach.hpp>


/*
    I want to integrate the ngram2/l implementation done by erikf
    in moost here. This is a bit hacky, but gets the job done 99% for now.
*/

    
void
RS_local_library::init(playdar::Config * c, Resolver * r)
{
    m_resolver  = r;
    m_conf = c;
    m_exiting = false;
    cout << "Local library resolver: " << app()->library()->num_files() 
         << " files indexed." << endl;
    if(app()->library()->num_files() == 0)
    {
        cout << endl << "WARNING! You don't have any files in your database!"
             << "Run the scanner, then restart Playdar." << endl << endl;
    }
    // worker thread for doing actual resolving:
    m_t = new boost::thread(boost::bind(&RS_local_library::run, this));
}

void
RS_local_library::start_resolving( rq_ptr rq )
{
    boost::mutex::scoped_lock lk(m_mutex);
    m_pending.push_front( rq );
    m_cond.notify_one();
}

/// thread that loops forever processing incoming queries:
void
RS_local_library::run()
{
    try
    {
        rq_ptr rq;
        while(true)
        {
            {
                boost::mutex::scoped_lock lk(m_mutex);
                if(m_pending.size() == 0) m_cond.wait(lk);
                if(m_exiting) break;
                rq = m_pending.back();
                m_pending.pop_back();
            }
            process( rq );
        }
    }
    catch(...)
    {
        cout << "RS_local_library runner exiting." << endl;
    }
}

/// this is some what fugly atm, but gets the job done for now.
/// it does the fuzzy library search using the ngram table from the db:
void
RS_local_library::process( rq_ptr rq )
{
    vector< boost::shared_ptr<PlayableItem> > final_results;
    
    // get candidates (rough potential matches):
    vector<scorepair> candidates = find_candidates(rq, 10);
    // now do the "real" scoring of candidate results:
    string reason; // for scoring debug.
    BOOST_FOREACH(scorepair &sp, candidates)
    {
        // multiple files in our collection may have matching metadata.
        // add them all to the results.
        vector<int> fids = app()->library()->get_fids_for_tid(sp.id);
        BOOST_FOREACH(int fid, fids)
        {
            pi_ptr pip = app()->library()->playable_item_from_fid(fid);
            float finalscore = m_resolver->calculate_score(rq, pip, reason);
            if(finalscore < 0.1) continue;
            pip->set_score(finalscore);
            pip->set_source(conf()->name());
            final_results.push_back( pip );
        }
    }
    if(final_results.size())
    {
        report_results(rq->id(), final_results, name());
    }
}

/// Search library for candidates roughly matching the query.
/// This works with track ids and associated metadata. It's possible
/// that our library has many files for the same track id (ie, same metadata)
/// this is of no concern to this method.
///
/// First find suitable artists, then collect matching tracks for each artist.
vector<scorepair> 
RS_local_library::find_candidates(rq_ptr rq, unsigned int limit)
{ 
    vector<scorepair> candidates;
    float maxartscore = 0;
    vector<scorepair> artistresults =
        app()->library()->search_catalogue("artist", rq->artist());
    BOOST_FOREACH( scorepair & sp, artistresults )
    {
        if(maxartscore==0) maxartscore = sp.score;
        float artist_multiplier = (float)sp.score / maxartscore;
        float maxtrkscore = 0;
        vector<scorepair> trackresults = 
            app()->library()->search_catalogue_for_artist(sp.id, 
                                                          "track",
                                                          rq->track());
        BOOST_FOREACH( scorepair & sptrk, trackresults )
        {
            if(maxtrkscore==0) maxtrkscore = sptrk.score;
            float track_multiplier = (float) sptrk.score / maxtrkscore;
            // combine two scores:
            float combined_score = artist_multiplier * track_multiplier;
            scorepair cand;
            cand.id = sptrk.id;
            cand.score = combined_score;
            candidates.push_back(cand);
        } 
    }
    // sort candidates by combined score
    sort(candidates.begin(), candidates.end(), sortbyscore()); 
    if(limit > 0 && candidates.size()>limit) candidates.resize(limit);
    return candidates;
}


