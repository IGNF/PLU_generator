#ifndef GRAPH_CONFIGURATION_PLU_HPP
#define GRAPH_CONFIGURATION_PLU_HPP

#include <boost/graph/adjacency_list.hpp>
//#include "rjmcmc/mpp/configuration/configuration.hpp"
#include "rjmcmc/util/variant.hpp"
#include "configuration_plu.hpp"

namespace marked_point_process
{

template<typename T
, typename UnaryDistBorder
, typename BinaryDistPair
, typename BinaryHeightDiff
, typename GlobalLCR
, typename GlobalFAR
, typename Accelerator=trivial_accelerator, typename OutEdgeList = boost::listS, typename VertexList = boost::listS  >

class graph_configuration_plu
{
public:
    typedef graph_configuration_plu<T,UnaryDistBorder, BinaryDistPair, BinaryHeightDiff, GlobalLCR,GlobalFAR, Accelerator, OutEdgeList, VertexList> self;

    typedef T	value_type;
private:
    class edge
    {
        double m_energy;
    public:
        edge() : m_energy(0) {}
        inline double energy() const {return m_energy;}
        inline void energy(double e){m_energy = e;}
    };

    class node
    {
        value_type	m_value;
        double      m_energy;
    public:
        node(const value_type& obj, double e) : m_value(obj), m_energy(e) { }
        inline const value_type& value() const{return m_value;}
        inline double energy() const{return m_energy;}
    };
    typedef boost::adjacency_list<OutEdgeList, VertexList, boost::undirectedS, node, edge> graph_type;
    typedef typename graph_type::out_edge_iterator	out_edge_iterator;
    typedef	typename graph_type::vertex_descriptor	vertex_descriptor;
    typedef std::pair< typename graph_type::edge_descriptor , bool > edge_descriptor_bool;

public:
    typedef	typename graph_type::vertex_iterator	iterator;
    typedef	typename graph_type::vertex_iterator	const_iterator;
    typedef typename graph_type::edge_iterator	edge_iterator;
    typedef typename graph_type::edge_iterator	const_edge_iterator;
    typedef internal::modification<self>	modification;
public:

    // configuration constructors/destructors
    graph_configuration_plu(UnaryDistBorder unary_dBorder
                            , BinaryDistPair binary_dPair
                            , BinaryHeightDiff binary_hDiff
                            , GlobalLCR global_lcr
                            , GlobalFAR global_far
                            , Accelerator accelerator=Accelerator())
        : m_unary_dBorder(unary_dBorder)
        , m_binary_dPair(binary_dPair)
        , m_binary_hDiff(binary_hDiff)
        , m_global_lcr(global_lcr)
        , m_global_far(global_far)
        , m_accelerator(accelerator)
        , m_dBorder(0.)
        , m_dPair(0.)
        , m_hDiff(0.)
        , m_delta_dBorder(0.)
        , m_delta_dPair(0.)
        , m_delta_hDiff(0.)
        , m_delta_lcr(0.)
        , m_delta_far(0.)
        , m_num(0)
    {
        m_lcr = audit_lcr();
        m_far = audit_far();
        m_death_dBorder = rjmcmc::apply_visitor(unary_dBorder,2,0);
    }

    ~graph_configuration_plu() {}

    // values
    inline size_t size() const{return num_vertices(m_graph);}
    inline bool empty() const{return (num_vertices(m_graph)==0);}
    inline iterator begin(){return vertices(m_graph).first;}
    inline iterator end  (){return vertices(m_graph).second;}
    inline const_iterator begin() const{return vertices(m_graph).first;}
    inline const_iterator end  () const{return vertices(m_graph).second;}
    inline const value_type& operator[]( const_iterator v ) const{return m_graph[ *v ].value();}
    inline const value_type& value( const_iterator v ) const{return m_graph[ *v ].value();}
    inline double energy( const_iterator v ) const{return m_graph[ *v ].energy();}

    // interactions
    inline size_t size_of_interactions   () const{return num_edges(m_graph);}
    inline edge_iterator interactions_begin(){return edges(m_graph).first;}
    inline edge_iterator interactions_end  (){return edges(m_graph).second;}
    inline const_edge_iterator interactions_begin() const{return edges(m_graph).first;}
    inline const_edge_iterator interactions_end  () const{return edges(m_graph).second;}
    inline double energy( edge_iterator e ) const{return m_graph[ *e ].energy();}

    // evaluators

    template <typename Modification> double delta_energy(const Modification &modif)
    {
        clear_delta();
        typedef typename Modification::birth_type::const_iterator bci; //iterator of value_type (object)
        typedef typename Modification::death_type::const_iterator dci; //iterator of vertex_iterator
        bci bbeg = modif.birth().begin();
        bci bend = modif.birth().end();
        dci dbeg = modif.death().begin();
        dci dend = modif.death().end();

        for(bci it=bbeg; it!=bend; ++it)
        {
            m_delta_dBorder += rjmcmc::apply_visitor(m_unary_dBorder,*it);

            const_iterator   it2, end2;
            boost::tie(it2,end2)=m_accelerator(*this,*it); //vertex iterator [first, end)
            for (; it2 != end2; ++it2)
                if (std::find(dbeg,dend,it2)==dend) //it2 not dead
                {
                    m_delta_dPair += rjmcmc::apply_visitor(m_binary_dPair, *it, value(it2) );
                    m_delta_hDiff += rjmcmc::apply_visitor(m_binary_hDiff,*it,value(it2));
                }

            for (bci it2=bbeg; it2 != it; ++it2)
            {
                m_delta_dPair += rjmcmc::apply_visitor(m_binary_dPair, *it, *it2);
                m_delta_hDiff += rjmcmc::apply_visitor(m_binary_hDiff,*it,*it2);
            }

        }

        int flagIntersect = 0;
        for(dci it=dbeg; it!=dend; ++it)
        {
            iterator v = *it;
            m_delta_dBorder -= energy(v);

            const_iterator   it2, begin2=begin(),end2=end();

            for (it2=begin(); it2 != v; ++it2)
                if(std::find(dbeg,dend,it2)==dend)
                {
                    m_delta_dPair -= rjmcmc::apply_visitor(m_binary_dPair, value(v), value(it2) );
                    m_delta_hDiff -= rjmcmc::apply_visitor(m_binary_hDiff,value(v),value(it2));
                    flagIntersect |= geometry::do_intersect(value(v),value(it2));
                }
            it2 = v;
            ++it2;
            for(; it2!=end2; ++it2)
            {
                m_delta_dPair -= rjmcmc::apply_visitor(m_binary_dPair, value(v), value(it2) );
                m_delta_hDiff -= rjmcmc::apply_visitor(m_binary_hDiff,value(v),value(it2));
                flagIntersect |= geometry::do_intersect(value(v),value(it2));
            }

        }

        m_delta_lcr += rjmcmc::apply_visitor(m_global_lcr,*this,modif) - m_lcr;
        m_delta_far += rjmcmc::apply_visitor(m_global_far,*this,modif) - m_far;


        if(int n = modif.death().size())
        {
            if(std::abs(m_delta_dBorder)>m_death_dBorder*n)
                return m_delta_dBorder;
//            if(flagIntersect)
//                return m_delta_dPair;
        }

        return m_delta_dBorder+m_delta_dPair+m_delta_hDiff+m_delta_lcr+m_delta_far;
    }

    template <typename Modification> void apply(const Modification &modif)
    {
        typedef typename Modification::birth_type::const_iterator bci;
        typedef typename Modification::death_type::const_iterator dci;

        dci dbeg = modif.death().begin();
        dci dend = modif.death().end();
        for(dci dit=dbeg; dit!=dend; ++dit) //remove(*dit);
        {
            clear_vertex ( **dit , m_graph);
            remove_vertex( **dit , m_graph);
            m_num--;
        }
        bci bbeg = modif.birth().begin();
        bci bend = modif.birth().end();
        for(bci bit=bbeg; bit!=bend; ++bit) //insert(*bit);
        {
            node n(*bit, rjmcmc::apply_visitor(m_unary_dBorder,*bit));
            add_vertex(n, m_graph);
            m_num++;
        }

        m_dBorder += m_delta_dBorder;
        m_dPair = audit_dPair();
        m_hDiff = audit_hDiff();
//        m_lcr = audit_lcr();
//        m_far = audit_far();
        m_lcr += m_delta_lcr;
        m_far += m_delta_far;
    }

    inline void clear()
    {
        m_graph.clear();
        m_dBorder=m_dPair=m_hDiff=m_lcr=m_far=0.;
    }

    inline void clear_delta()
    {
        m_delta_lcr=0.;
        m_delta_far=0.;
        m_delta_dBorder=0.;
        m_delta_dPair=0.;
        m_delta_hDiff = 0.;
    }

    //configuration accessors
    inline double energy_dBorder() const{return m_dBorder;}
    inline double energy_dPair  () const{return m_dPair; }
    inline double energy_hDiff  () const{return m_hDiff;}
    inline double energy_lcr    () const{return m_lcr;}
    inline double energy_far    () const{return m_far;}
    inline double energy        () const{return m_dBorder+m_dPair+m_hDiff+m_lcr+m_far;}
    inline double unary_energy  () const{return m_dBorder;}
    inline double binary_energy () const{return m_dPair+m_hDiff;}
    inline int    getNumObjects () const{return m_num;}

    inline double audit_dBorder() const
    {
        double e = 0.;
        for (const_iterator i=begin(); i != end(); ++i)
            e += rjmcmc::apply_visitor(m_unary_dBorder, value(i) );
        return e;
    }

    inline double audit_dPair() const
    {
        double e = 0.;

        if(size()>1)
            for (const_iterator i=begin(); i != end(); ++i)
            {
                const_iterator j=i;
                j++;
                for(; j!=end(); ++j)
                    e += rjmcmc::apply_visitor(m_binary_dPair, value(i),value(j) );
            }
        return e;
    }

    inline double audit_hDiff() const
    {
        double e = 0.;

        if(size()>1)
            for (const_iterator i=begin(); i != end(); ++i)
            {
                const_iterator j=i;
                j++;
                for(; j!=end(); ++j)
                    e += rjmcmc::apply_visitor(m_binary_hDiff, value(i),value(j) );
            }
        return e;
    }

    inline double audit_lcr() const
    {
        modification modif;
        return rjmcmc::apply_visitor(m_global_lcr,*this,modif);
    }

    inline double audit_far() const
    {
        modification modif;
        return rjmcmc::apply_visitor(m_global_far,*this,modif);
    }


    inline double audit_energy() const{return audit_dBorder() + audit_dPair() + audit_hDiff() + audit_lcr()+audit_far();}
    inline double audit_unary_energy() const{return audit_dBorder();}
    inline double audit_binary_energy() const{return audit_dPair() + audit_hDiff();}
    unsigned int audit_structure() const{return 0;}

private:
    graph_type m_graph;
    UnaryDistBorder	m_unary_dBorder;
    BinaryDistPair  m_binary_dPair;
    BinaryHeightDiff m_binary_hDiff;
    GlobalLCR       m_global_lcr;
    GlobalFAR       m_global_far;
    Accelerator	m_accelerator;

    double m_dBorder;
    double m_dPair;
    double m_hDiff;
    double m_lcr;
    double m_far;

    double m_delta_dBorder;
    double m_delta_dPair;
    double m_delta_hDiff;
    double m_delta_lcr;
    double m_delta_far;

    double m_death_dBorder;
    int m_num;
};

}; // namespace marked_point_process

#endif // GRAPH_CONFIGURATION_PLU_HPP
