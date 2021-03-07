#include <queue>
#include <iostream>
#include "utilities.h"

std::pair<regex::table, regex::group> generate_nfa_table(std::shared_ptr<regex::fa> nfa)
{
    regex::table table;
    regex::group start;

    nfa->walk([&table, &start](std::weak_ptr<regex::state> state) {
        if (start.empty())
        {
            start = state.lock()->get_epsilon_closure();
        }

        if (table.find(regex::group({state.lock()})) != std::cend(table))
        {
            return;
        }
        else
        {
            for (auto &transition : state.lock()->get_transitions())
            {
                table.update(regex::group({state.lock()}), transition.first, transition.second);
            }
        }
    });

    return {table, start};
}

std::pair<regex::table, regex::group> generate_dfa_table(std::shared_ptr<regex::fa> nfa)
{
    regex::table nfa_table, dfa_table;
    regex::group start;

    std::tie(nfa_table, start) = generate_nfa_table(nfa);

    // std::cout << nfa_table <<std::endl;

    auto do_one_row = [ &nfa_table, &dfa_table](regex::group start)
    {
        if(dfa_table.find(start) != std::cend(dfa_table))
        {
            return std::vector<regex::group>();
        }
        else
        {
            auto schema = nfa_table.get_schema();
            auto epsilon_col = schema.find(0x01);

            std::vector<regex::group> next_dfa_states;

            for(const auto state: start)
            {
                auto row = nfa_table.find(regex::group({state}))->second;

                for(int col = 0; col < schema.size(); ++col)
                {
                    if( !row[col].empty() && col != epsilon_col )
                    {
                        auto transition_character = schema[col];
                        auto transition_group = row[col];

                        for( auto transition_state : transition_group)
                        {
                            auto dfa_state = nfa_table.find(regex::group({transition_state}))->second[epsilon_col];

                            dfa_table.update(regex::group({start}), transition_character, dfa_state);
                            next_dfa_states.push_back(dfa_state);
                        }
                    }
                }
            }

            return next_dfa_states;
        }        
    };

    std::queue<regex::group> assembly_line;

    assembly_line.push(start);

    while( !assembly_line.empty())
    {
        auto next_states = do_one_row(assembly_line.front());

        assembly_line.pop();

        for(auto next: next_states)
        {
            assembly_line.push(next);
        }
    }

    return {dfa_table, start};
}