#ifndef _SERIALIZE_H_
#define _SERIALIZE_H_

#include <string>
#include <vector>

/**
 * This file defines the core set of objects that define the
 * Serialization layer.
 */
class Serialize;
class SerializeAction;
class SerializableObject;


//XXX/bowei namespace
namespace oasys {
    typedef ::Serialize          Serialize;
    typedef ::SerializeAction    SerializeAction;
    typedef ::SerializableObject SerializableObject;
};

/**
 * Empty base class that's just used for name scoping of the action
 * and context enumerated types.
 */
class Serialize {
public:
    /**
     * Action type codes, one for each basic type of SerializeAction.
     */
    typedef enum {
        MARSHAL = 1,	/// in-memory  -> serialized representation
        UNMARSHAL,	/// serialized -> in-memory  representation
        INFO		/// informative scan (e.g. size, table schema)
    } action_t;
    
    /**
     * Context type codes, one for each general context in which
     * serialization occurs. 
     */
    typedef enum {
        CONTEXT_UNKNOWN = 1,	/// no specified context (default)
        CONTEXT_NETWORK,	/// serialization to/from the network
        CONTEXT_LOCAL		/// serialization to/from local disk
    } context_t;
};    

/**
 * Inherit from this class to add serialization capability to a class.
 */
class SerializableObject {
public:
    /**
     * This should call v->process() on each of the types that are to
     * be serialized in the object.
     */
    virtual void serialize(SerializeAction* a) = 0;
};

/**
 * A vector of SerializableObjects.
 */
typedef std::vector<SerializableObject*> SerializableObjectVector;

/**
 * The SerializeAction is responsible for implementing callback
 * functions for all the basic types. The action object is then passed
 * to the serialize() function which will re-dispatch to the basic
 * type functions for each of the SerializableObject's member fields.
 *
 * INVARIANT: A single SerializeAction must be able to be called on
 * several different visitee objects in succession. (Basically this
 * ability is used to be able to string several Marshallable objects
 * together, either for writing or reading).
 */
class SerializeAction : public Serialize {
public:
    /**
     * Create a SerializeAction with the specified type code and context
     *
     * @param type serialization action type code
     * @param context serialization context
     */
    SerializeAction(action_t action, context_t context);

    /**
     * Perform the serialization or deserialization action on the object.
     *
     * @return 0 if success, -1 on error
     */
    virtual int action(SerializableObject* object);

    /**
     * Control the initialization done before executing an action.
     */
    virtual void begin_action();

    /**
     * Control the cleanup after executing an action.
     */
    virtual void end_action();

    /**
     * Accessor for the action type.
     */
    action_t action() { return action_; }
    
    /**
     * Accessor for the context.
     */
    context_t context() { return context_; }
    
    /***********************************************************************
     *
     * Processor functions, one for each type.
     *
     ***********************************************************************/

    /**
     * Process function for a contained SerializableObject.
     *
     * The default implementation just calls serialize() on the
     * contained object, ignoring the name value. However, a derived
     * class can of course override it to make use of the name (as is
     * done by SQLTableFormat, for example).
     */
    virtual void process(const char* name, SerializableObject* object)
    {
        object->serialize(this);
    }
    
    /**
     * Process function for a 4 byte integer.
     */
    virtual void process(const char* name, u_int32_t* i) = 0;

    /**
     * Process function for a 2 byte integer.
     */
    virtual void process(const char* name, u_int16_t* i) = 0;

    /**
     * Process function for a byte.
     */
    virtual void process(const char* name, u_int8_t* i) = 0;

    /**
     * Process function for a boolean.
     */
    virtual void process(const char* name, bool* b) = 0;

    /**
     * Process function for a constant length char buffer.
     */
    virtual void process(const char* name, u_char* bp, size_t len) = 0;

    /**
     * Process function for a variable length char buffer. The
     * alloc_copy flag dictates whether the unmarshal variants should
     * malloc() a new buffer in the target object.
     */
    virtual void process(const char* name, u_char** bp,
                         size_t* len, bool alloc_copy) = 0;

    /**
     * Process function for a c++ string.
     */
    virtual void process(const char* name, std::string* s) = 0;

    ///@{
    /**
     * Adaptor functions for signed/unsigned compatibility
     */
    virtual void process(const char* name, int32_t* i)
    {
        process(name, (u_int32_t*)i);
    }
    
    virtual void process(const char* name, int16_t* i)
    {
        process(name, (u_int16_t*)i);
    }

    virtual void process(const char* name, int8_t* i)
    {
        process(name, (u_int8_t*)i);
    }

    virtual void process(const char* name, char* bp, size_t len)
    {
        process(name, (u_char*)bp, len);
    }

    /// @}
    
    /** Set a log target for verbose serialization */
    void logpath(const char* log) { log_ = log; }
    
    /**
     * Destructor.
     */
    virtual ~SerializeAction();

protected:
    action_t action_;	///< Serialization action code
    context_t context_;	///< Serialization context
    bool  error_;	///< Indication of whether an error occurred
    const char* log_;	///< Optional log for verbose marshalling

private:
    SerializeAction();	/// Never called
};

#endif /* _SERIALIZE_H_ */
