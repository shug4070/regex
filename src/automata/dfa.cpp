#include "automata/dfa.h"

#include <iostream>
#include <memory>
#include <queue>

#include "automata/nfa.h"
#include "state/dstate.h"
#include "utilities.h"

namespace regex
{
    dfa::dfa( std::shared_ptr<dstate> input, const std::set<std::shared_ptr<dstate>> &outputs ) : input_( input ), outputs_( outputs )
    {
    }

    dfa::dfa( const dfa &other ) : input_( ), outputs_( )
    {
        std::tie( input_, outputs_ ) = duplicate( other.input_ );
    }

    std::shared_ptr<dfa> dfa::from_character( language::character_type c )
    {
        auto input = std::make_shared<dstate>( state::context::rejecting );
        auto output = std::make_shared<dstate>( state::context::accepting );

        input->connect( c, output );

        return std::make_shared<dfa>( input, std::set<std::shared_ptr<dstate>>{ output } );
    }

    std::shared_ptr<dfa> dfa::from_any( )
    {
        auto input = std::make_shared<dstate>( state::context::rejecting );
        auto output = std::make_shared<dstate>( state::context::accepting );

        for( const auto letter : language::alphabet )
        {
            input->connect( letter, output );
        }

        return std::make_shared<dfa>( input, std::set<std::shared_ptr<dstate>>{ output } );
    }

    std::shared_ptr<dfa> dfa::from_concatenation( std::shared_ptr<dfa> lhs, std::shared_ptr<dfa> rhs )
    {
        for( auto &lhs_output : lhs->outputs_ )
        {
            shallow_copy( rhs->input_, lhs_output, std::set<std::shared_ptr<dstate>>( ) );

            if( rhs->input_->get_type( ) == state::context::rejecting )
            {
                lhs_output->set( state::context::rejecting );
            }
        }

        std::set<std::shared_ptr<dstate>> new_outputs;

        for( const auto &old_output : rhs->outputs_ )
        {
            if( old_output == rhs->input_ )
            {
                new_outputs.merge( lhs->outputs_ );
            }
            else
            {
                new_outputs.insert( old_output );
            }
        }

        return std::make_shared<dfa>( lhs->input_, new_outputs );
    }

    std::shared_ptr<dfa> dfa::from_alternation( std::shared_ptr<dfa> lhs, std::shared_ptr<dfa> rhs )
    {
        merge( lhs->input_, rhs->input_ );
        rhs->outputs_.merge( lhs->outputs_ );
        return std::make_shared<dfa>( rhs->input_, rhs->outputs_ );
    }

    std::shared_ptr<dfa> dfa::from_kleene( std::shared_ptr<dfa> expression )
    {
        for( auto &output : expression->outputs_ )
        {
            shallow_copy( expression->input_, output, std::set<std::shared_ptr<dstate>>( ) );
        }

        expression->input_->set( state::context::accepting );
        expression->outputs_.insert( expression->input_ );

        return std::make_shared<dfa>( expression->input_, expression->outputs_ );
    }

    std::shared_ptr<dfa> dfa::from_zero_or_one( std::shared_ptr<dfa> expression )
    {
        expression->input_->set( state::context::accepting );

        return expression;
    }

    match dfa::execute( std::basic_string_view<language::character_type> target )
    {
        return input_->execute( target );
    }

    void dfa::walk( std::function<void( std::shared_ptr<state> )> callback )
    {
        input_->walk( callback );
    }
}