#include <string_view>
#include <memory>
#include <map>
#include <algorithm>
#include <vector>
#include <set>

#include "state/dstate.h"

namespace regex
{
    dstate::dstate(state::context ctx)
        : state(ctx),
          transitions_()
    {
    }

    void dstate::connect(regex::character_type symbol, std::shared_ptr<dstate> st)
    {
        transitions_[symbol] = st;
        set(state::context::rejecting);
    }

    void merge(std::shared_ptr<dstate> source, std::shared_ptr<dstate> target)
    {
        target->set(source->get_type() == state::context::accepting || target->get_type() == state::context::accepting ? state::context::accepting : state::context::rejecting);

        target->transitions_.merge(source->transitions_);

        for (const auto &source_transition : source->transitions_)
        {
            merge(source_transition.second, target->transitions_.find(source_transition.first)->second);
        }
    }

    void copy(std::shared_ptr<const dstate> source, std::shared_ptr<dstate> target)
    {
        for (const auto &source_transition : source->transitions_)
        {
            const auto &result = target->transitions_.insert(source_transition);
            const auto &existed = !result.second;
            const auto &position = result.first;

            if (existed)
            {
                copy(source_transition.second, position->second);
            }
        }
    }

    group dstate::get_transitions(regex::character_type symbol)
    {
        auto ptr = transitions_.find(symbol);

        if(ptr == std::cend(transitions_))
        {
            return group();
        }
        else
        {
            return group{ ptr->second };
        }        
    }

    group dstate::get_epsilon_closure()
    {
        return group();
    }

    std::map<character_type, group> dstate::get_transitions()
    {
        std::map<character_type, group> result;

        for (const auto &transition : transitions_)
        {
            result.insert({transition.first, group{ transition.second }});
        }

        return result;
    }

    match dstate::next(std::basic_string_view<regex::character_type> str)
    {
        if (str.empty())
        {
            return get_type() == state::context::accepting ? match::accepted : match::rejected;
        }

        if (transitions_.find(str[0]) != std::cend(transitions_))
        {
            return transitions_.find(str[0])->second->next(str.substr(1));
        }
        else
        {
            return match::rejected;
        }
    }

    void dstate::walk(std::function<void(std::shared_ptr<state>)> callback, std::set<std::shared_ptr<state>> visited)
    {
        if (visited.find(shared_from_this()) != std::end(visited))
        {
            return;
        }
        else
        {
            visited.insert(shared_from_this());

            auto temp = shared_from_this();

            callback(temp);

            for (auto &transitionForCharacter : transitions_)
            {
                transitionForCharacter.second->walk(callback, visited);
            }
        }
    }
} // namespace fa