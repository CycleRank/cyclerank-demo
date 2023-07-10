package main

import (
	"errors"
	"math/big"
	"net/url"
	"os"
	"regexp"
	"strconv"

	"github.com/google/uuid"
)

// API error on POST
var (
	ErrRequiredField             = errors.New("field is required")
	ErrNotValidPath              = errors.New("not valid path")
	ErrNotPositiveInt            = errors.New("not a positive integer")
	ErrNotUint                   = errors.New("not an unsigned integer")
	ErrNotFloatBetweenZeroAndOne = errors.New("not a float between 0.0 and 1.0")
	ErrNotSafeFileName           = errors.New("not a safe filename [a-zA-Z0-9./]+")
	ErrIncorrectScoringFunction  = errors.New("not a scoring function")
	ErrInvalidUuid               = errors.New("not a valid uuid")
)

// ErrOptionalNotProvided used internally to break validation loop
var ErrOptionalNotProvided = errors.New("optional field not provided")

// ValidationFunction is a function that validates a property of a field.
type ValidationFunction func(f url.Values, key string) error

// Form is the abstraction of a real form; maps field to validation functions.
type Form map[string][]ValidationFunction

// Validate check the supplied form data against the Form structure.
func (f Form) Validate(data url.Values) map[string]string {
	errors := map[string]string{}
	for key, validators := range f {
		for i := 0; i < len(validators); i++ {
			if err := validators[i](data, key); err != nil {
				if err != ErrOptionalNotProvided {
					errors[key] = err.Error()
				}
				break
			}
		}
	}

	return errors
}

// FieldRequired assert the presence of a value
func FieldRequired(f url.Values, key string) error {
	if !FormIsSet(f, key) {
		return ErrRequiredField
	}
	return nil
}

// FieldOptional breaks the validation loop if a value is not supplied
func FieldOptional(f url.Values, key string) error {
	if !FormIsSet(f, key) {
		return ErrOptionalNotProvided
	}
	return nil
}

// FieldSafePathRegexp "safe file name" constraint.
var FieldSafePathRegexp = regexp.MustCompile("[a-zA-Z0-9/.]+")

// FieldSafePath asserts that the file name is safe enough.
func FieldSafePath(f url.Values, key string) error {
	if !FieldSafePathRegexp.Match([]byte(f[key][0])) {
		return ErrNotSafeFileName
	}
	return nil
}

// FieldFileExists asserts that the field points to an existing and readable file.
func FieldFileExists(f url.Values, key string) error {
	path := SafeFilePath(*repository, f[key][0])
	// NOTE: We assume that in the repository we have only readable files
	//  and directories.

	if info, err := os.Stat(path); err != nil || info.IsDir() {
		return ErrNotValidPath
	}
	return nil
}

// FieldUint asserts that the field value
//
//	is a striclty positive integer
func FieldUint(f url.Values, key string) error {
	if _, err := strconv.ParseUint(f[key][0], 10, 64); err != nil {
		return ErrNotUint
	}
	return nil
}

// FieldInt asserts that the field value
//
//	is an integer
func FieldInt(f url.Values, key string) error {
	if _, err := strconv.ParseInt(f[key][0], 10, 64); err != nil {
		return ErrNotUint
	}
	return nil
}

// FieldStrictlyPositiveInt asserts that the field value
//
//	is a striclty positive integer
func FieldStrictlyPositiveInt(f url.Values, key string) error {
	if num, err := strconv.ParseUint(f[key][0], 10, 64); err != nil || num <= 0 {
		return ErrNotPositiveInt
	}
	return nil
}

// FieldBoolean asserts that the field value is indeed a boolean
func FieldBoolean(f url.Values, key string) error {
	_, err := strconv.ParseBool(f[key][0])
	return err
}

// FieldFloatBetweenZeroAndOne asserts that the field value is a float between 0.0 and 1.0 (extremes included)
func FieldFloatBetweenZeroAndOne(f url.Values, key string) error {
	if num, err := strconv.ParseFloat(f[key][0], 64); err != nil || big.NewFloat(num).Cmp(big.NewFloat(0.0)) < 0 || big.NewFloat(num).Cmp(big.NewFloat(1.0)) > 0 {
		return ErrNotFloatBetweenZeroAndOne
	}
	return nil
}

// FieldCorrectScoringFunction asserts that the field value is either exp, lin, quad or log
func FieldCorrectScoringFunction(f url.Values, key string) error {
	if f[key][0] != "exp" && f[key][0] != "lin" && f[key][0] != "quad" && f[key][0] != "log" {
		return ErrIncorrectScoringFunction
	}
	return nil
}

func FieldIsUuid(f url.Values, key string) error {
	_, err := uuid.Parse(f[key][0])
	if err != nil {
		return ErrInvalidUuid
	}
	return nil
}

// FormIsSet checks wheter a field has been provided in a form
func FormIsSet(f url.Values, key string) bool {
	if file, exists := f[key]; exists && len(file) > 0 {
		return true
	}
	return false
}
