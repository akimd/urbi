/*
 * Copyright (C) 2011, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

/**
 ** \file runner/job.hh
 ** \brief Definition of runner::Job.
 */

#ifndef RUNNER_JOB_HH
# define RUNNER_JOB_HH

// declare sched::Job
# include <sched/job.hh>

// declare runner::State
# include <runner/state.hh>

// declare object::*
# include <urbi/object/fwd.hh>
# include <object/profile.hh>

// declare evalution interface eval::Action
# include <eval/action.hh>

// Avoid post-declaration of runner::Job
# include <runner/fwd.hh>

// Register events used to watch for future changes of the evaluation
// result.  \a Name corresponds to an attribute which of the object
// accessible through an accessor Name_get().
#define URBI_AT_HOOK(Name)                                              \
  do                                                                    \
  {                                                                     \
    if (runner::Job* r =                                                \
        ::kernel::urbiserver->getCurrentRunnerOpt())                    \
      if (r->dependencies_log_get())                                    \
      {                                                                 \
        try                                                             \
        {                                                               \
          r->dependencies_log_set(false);                               \
          GD_CATEGORY(Urbi.At);                                         \
          GD_FPUSH_TRACE("Register %s for at monitoring", #Name);       \
          rEvent e = Name##_get();                                      \
          r->dependencies_log_set(true);                                \
          r->dependency_add(e);                                         \
        }                                                               \
        catch (...)                                                     \
        {                                                               \
          r->dependencies_log_set(true);                                \
          throw;                                                        \
        }                                                               \
      }                                                                 \
  }                                                                     \
  while (false)

namespace runner
{
  using urbi::object::Profile;
  using urbi::object::FunctionProfile;

  /// Job is holding a common stack representation for all method of
  /// evaluations.  Method of evaluation could be Text evaluation, AST
  /// evaluation, or Byte code evaluation.
  class Job
    : public sched::Job
  {
  public:
    typedef sched::Job super_type;

    typedef object::Object Object;
    typedef object::rObject rObject;
    typedef object::rLobby rLobby;

    /// Create a job from another one.
    ///
    /// \param model The parent job. The scheduler and tags will be inherited
    //         from it.
    ///
    /// \param name The name of the new job, or a name derived from \a model
    ///        if none is provided.
    Job(const Job& model);

    /// Create a new job.
    ///
    /// \param lobby The lobby attached to the current job.
    ///
    /// \param scheduler The scheduler to which this job will be attached.
    ///
    /// \param name The name of the new job, or an automatically created
    ///        one if none is provided.
    Job(rLobby lobby,
        sched::Scheduler& scheduler);

    /// Destroy a job and the action bound to it.
    virtual ~Job();

  public:

    /// Urbiscript evaluation
    /// \{

    /// Urbiscript is evaluated in this function, whatever the
    /// representation used to store urbiscript sources.
    virtual void work();

    /// Return the result of the latest evaluation.
    /// FIXME: is that used ?!
    virtual rObject result_get();

    /// Get the current runner as an Urbi Job or nil if the job is
    /// terminated.
    rObject as_job();

    /// Signal a scheduling error exception.
    /// Implementation for abstract class sched::Job.
    ///
    /// \param msg The explanation of the scheduling error.
    virtual void scheduling_error(const std::string& msg);

    // Avoid cycle dependencies with cached objects.
    virtual void terminate_cleanup();

    /// \}


    /// Job status defined by Tags
    /// \{

    /// Inherited from sched::job.
    /// Bounce on the state to know if one tag is frozen.
    virtual bool frozen() const;

    /// Freeze the current job, without using any tag.
    void frozen_set(bool);

    /// Inherited from sched::job.
    /// Bounce on the state to know if the job has a specific tag.
    virtual size_t has_tag(const sched::Tag& tag,
			   size_t max_depth = (size_t)-1) const;
    bool has_tag(const object::rTag& tag) const;

    /// Inherited from sched::job.
    /// Bounce on the state to know the higest tag priority.
    virtual sched::prio_type prio_get() const;

    /// \}

    Job* name_set(const std::string& name);
    const std::string name_get() const;

    /// Job processing
    /// \{

    /// Create a new Job which inherits the current lobby and clone
    /// necessary stacks parts.  Define an action which would be executed by
    /// the child, and if a collector is specified, then the child is
    /// registered in the collector.
    ///
    /// This command return a child job which has to be started with
    /// 'start_job()' to execute the action asynchronously.
    Job* spawn_child(eval::Action action,
                     Job::Collector& collector);
    Job* spawn_child(eval::Action action);

    void set_action(eval::Action action);

    /// \}

    /// \name Profiling
    /// \{

    /// Start profiling into \a profile.
    /// profile_start should be used with profile_stop.
    void profile_start(Profile* profile, libport::Symbol name,
                       Object* current, bool count = false);
    /// Stop profiling.
    /// profile_stop should be used with profile_start.
    void profile_stop();

    /// Fork the profiling content of the parent
    /// profile_fork should be used with profile_join.
    void profile_fork(Job& parent);
    /// Append profiling content of a child.
    /// profile_join should be used with profile_fork.
    void profile_join(Job& child);

    ///
    Profile::idx profile_enter(Object* function, libport::Symbol msg);
    ///
    void profile_leave(Profile::idx idx);

    /// Wheter there is a profiling in run.
    bool is_profiling() const;

    /// The current profile to fill - if any.
    ATTRIBUTE_R(Profile*, profile);
  private:
    mutable Profile::Info profile_info_;

  protected:
    virtual void hook_preempted() const;
    virtual void hook_resumed() const;

    /// \}

    /// \name Dependencies tarcker
    /// \{

  public:
    typedef boost::unordered_set<object::rEvent> dependencies_type;
    dependencies_type& dependencies();
    bool dependencies_log_get() const;
    void dependencies_log_set(bool val);
    void dependency_add(object::rEvent evt);
    void dependencies_clear();
  protected:
    bool dependencies_log_;
    dependencies_type dependencies_;

    /// \}

    /// \name sched::Job accessors
    /// \{

  public:
    // FIXME: should ensure that one job release the non-interruptible flag.
    bool non_interruptible_get() const;
    void non_interruptible_set(bool);

    /// \}

  public:
    State state;

  private:
    eval::Action worker_;
    rObject result_cache_;
    rObject job_cache_;
  };

  /// Smart pointer shorthand
  typedef libport::intrusive_ptr<Job> rJob;

} // namespace runner

# if defined LIBPORT_COMPILATION_MODE_SPEED
#  include <runner/job.hxx>
# endif

#endif // ! RUNNER_JOB_HH